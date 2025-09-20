#include "../Base/stdredirector.h"
#include "LogWidget.h"
#include <ctime>
#include <fstream>
#include <iomanip>
#include <QDir>
#include <QFont>
#include <QScrollBar>
#include <QSizePolicy>
#include <sstream>

StdRedirector g_std_redirector;

static std::string GenerateLogFilePath() {
	auto t = std::time(nullptr);
	std::tm tm;
#ifdef _WIN32
	localtime_s(&tm, &t);
#else
	localtime_r(&t, &tm);
#endif
	std::ostringstream oss;
	oss << std::put_time(&tm, "%Y-%m-%d_%H-%M") << ".log";
	return oss.str();
}

LogWidget::LogWidget(QWidget* parent)
	: QWidget(parent)
	, m_textEdit(nullptr)
	, m_clearButton(nullptr)
	, m_layout(nullptr)
	, m_flushTimer(nullptr)
	, m_maxLines(80000)
	, m_logSink(nullptr)
	, m_fileThreadRunning(false)
	, m_logFileFolder("../log") // 默认日志文件名
{
	InitializeUI();
	InitializeLogging();
	StartLogFileThread();
}

LogWidget::~LogWidget()
{
	StopLogFileThread();
	if (m_logSink) {
		google::RemoveLogSink(m_logSink.get());
		m_logSink.reset();
	}
}

void LogWidget::InitializeUI()
{
	setWindowTitle("Log");
	setWindowFlags(Qt::Window);
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	setMinimumHeight(120);

	m_clearButton = new QPushButton("Clear", this);
	m_textEdit = new QPlainTextEdit(this);
	m_layout = new QGridLayout(this);
	m_flushTimer = new QTimer(this);

	m_textEdit->setReadOnly(true);
	m_textEdit->setMaximumBlockCount(m_maxLines);
	m_textEdit->setWordWrapMode(QTextOption::NoWrap);

	QFont font("Courier", 10);
	font.setStyleHint(QFont::Monospace);
	m_textEdit->setFont(font);

	m_layout->addWidget(m_clearButton, 0, 0, 1, 1, Qt::AlignLeft);
	m_layout->addWidget(m_textEdit, 1, 0, 1, 2);
	m_layout->setRowStretch(1, 1);
	m_layout->setColumnStretch(1, 1);

	setLayout(m_layout);

	connect(m_clearButton, &QPushButton::clicked, this, &LogWidget::Clear);
	connect(m_flushTimer, &QTimer::timeout, this, &LogWidget::Flush);

	m_flushTimer->start(100);
}

void LogWidget::InitializeLogging()
{
	enable_std_to_glog();
	m_logSink = std::make_unique<CustomLogSink>(this);
	google::AddLogSink(m_logSink.get());
}

void LogWidget::SetMaxLines(int maxLines)
{
	m_maxLines = maxLines;
	if (m_textEdit) {
		m_textEdit->setMaximumBlockCount(maxLines);
	}
}

void LogWidget::Append(const std::string& message, google::LogSeverity severity)
{
	// UI队列
	{
		QMutexLocker locker(&m_bufferMutex);
		m_logBuffer.push(std::make_pair(message, severity));
	}
	// 文件写线程队列
	{
		std::lock_guard<std::mutex> locker(m_fileQueueMutex);
		m_fileLogQueue.push(std::make_pair(message, severity));
		m_fileQueueCv.notify_one();
	}
}

QString LogWidget::GetLogPrefix(google::LogSeverity severity) const
{
	time_t now = time(nullptr);
	struct tm* tm_time = localtime(&now);

	char time_buf[32];
	strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_time);

	const char* severity_str;
	switch (severity) {
	case google::GLOG_WARNING:
		severity_str = "WARN";
		break;
	case google::GLOG_ERROR:
		severity_str = "ERROR";
		break;
	case google::GLOG_FATAL:
		severity_str = "FATAL";
		break;
	case google::GLOG_INFO:
	default:
		severity_str = "INFO";
		break;
	}
	return QString("[%1 %2] ").arg(time_buf).arg(severity_str);
}

void LogWidget::Clear()
{
	m_textEdit->clear();
	QMutexLocker locker(&m_bufferMutex);
	std::queue<std::pair<std::string, google::LogSeverity>> empty;
	m_logBuffer.swap(empty);
}

void LogWidget::Flush()
{
	std::queue<std::pair<std::string, google::LogSeverity>> localBuffer;
	{
		QMutexLocker locker(&m_bufferMutex);
		if (m_logBuffer.empty()) return;
		localBuffer.swap(m_logBuffer);
	}

	QScrollBar* scrollBar = m_textEdit->verticalScrollBar();
	bool shouldAutoScroll = (scrollBar->value() == scrollBar->maximum());

	while (!localBuffer.empty()) {
		auto logEntry = localBuffer.front();
		localBuffer.pop();

		QString prefix = GetLogPrefix(logEntry.second);
		QString message = QString::fromStdString(logEntry.first);

		if (message.endsWith('\n')) message.chop(1);

		QString fullMessage = prefix + message;
		m_textEdit->appendPlainText(fullMessage);
	}
	if (shouldAutoScroll) scrollBar->setValue(scrollBar->maximum());
}

void LogWidget::SaveLogToFile(const std::string& filePath)
{
	std::ofstream file(filePath, std::ios::out);
	CHECK(file.is_open()) << filePath;
	file << m_textEdit->toPlainText().toUtf8().constData();
}

void LogWidget::SetLogFilePath(const std::string& filePath)
{
	m_logFileFolder = filePath;
}

// ======================= 文件写线程实现 ==========================
void LogWidget::StartLogFileThread()
{
	m_fileThreadRunning = true;
	m_fileThread = std::thread([this] {
		std::string filePath = m_logFileFolder + "/" + GenerateLogFilePath();
		QDir dir(QString::fromStdString(m_logFileFolder));
		if (!dir.exists()) {
			dir.mkpath(".");
		}
		std::ofstream logFile(filePath, std::ios::app);
		if (!logFile.is_open()) return;

		while (m_fileThreadRunning) {
			std::unique_lock<std::mutex> lock(m_fileQueueMutex);
			m_fileQueueCv.wait(lock, [this] { return !m_fileLogQueue.empty() || !m_fileThreadRunning; });

			while (!m_fileLogQueue.empty()) {
				auto logEntry = m_fileLogQueue.front();
				m_fileLogQueue.pop();
				lock.unlock();

				// 写入文件
				QString prefix = this->GetLogPrefix(logEntry.second);
				QString message = QString::fromStdString(logEntry.first);
				if (message.endsWith('\n')) message.chop(1);

				QString fullMessage = prefix + message + "\n";
				logFile << fullMessage.toUtf8().constData();
				logFile.flush();

				lock.lock();
			}
		}
		while (!m_fileLogQueue.empty()) {
			auto logEntry = m_fileLogQueue.front();
			m_fileLogQueue.pop();

			QString prefix = this->GetLogPrefix(logEntry.second);
			QString message = QString::fromStdString(logEntry.first);
			if (message.endsWith('\n')) message.chop(1);

			QString fullMessage = prefix + message + "\n";
			logFile << fullMessage.toUtf8().constData();
		}
		logFile.flush();
		logFile.close();
		});
}

void LogWidget::StopLogFileThread()
{
	m_fileThreadRunning = false;
	m_fileQueueCv.notify_one();
	if (m_fileThread.joinable()) {
		m_fileThread.join();
	}
}

// ======================= CustomLogSink实现 ==========================
LogWidget::CustomLogSink::CustomLogSink(LogWidget* widget)
	: m_widget(widget)
{
}

void LogWidget::CustomLogSink::send(google::LogSeverity severity, const char* full_filename,
	const char* base_filename, int line,
	const struct ::tm* tm_time,
	const char* message, size_t message_len)
{
	if (m_widget) {
		std::string msg(message, message_len);
		m_widget->Append(msg, severity);
	}
}