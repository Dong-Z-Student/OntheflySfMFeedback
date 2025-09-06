#include "../Base/stdredirector.h"
#include "LogWidget.h"
#include <fstream>
#include <QApplication>
#include <QFont>
#include <QScrollBar>
#include <QSizePolicy>

StdRedirector g_std_redirector;

LogWidget::LogWidget(QWidget *parent)
    : QWidget(parent)
    , m_textEdit(nullptr)
    , m_clearButton(nullptr)
    , m_layout(nullptr)
    , m_flushTimer(nullptr)
    , m_maxLines(80000)
    , m_logSink(nullptr)
{
    InitializeUI();
    InitializeLogging();
}

LogWidget::~LogWidget()
{
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
    
    // 启动定时器，每100毫秒刷新一次
    m_flushTimer->start(100);
}

void LogWidget::InitializeLogging()
{
    // 启用std流转发，将所有std输出内容转发到LOG中，调试状态下请注释，不然崩溃会丢掉所有日志
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
    QMutexLocker locker(&m_bufferMutex);
    m_logBuffer.push(std::make_pair(message, severity));
}

QString LogWidget::GetLogPrefix(google::LogSeverity severity) const
{
    // 获取当前时间
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
        if (m_logBuffer.empty()) {
            return;
        }
        localBuffer.swap(m_logBuffer);
    }

    QScrollBar* scrollBar = m_textEdit->verticalScrollBar();
    bool shouldAutoScroll = (scrollBar->value() == scrollBar->maximum());

    while (!localBuffer.empty()) {
        auto logEntry = localBuffer.front();
        localBuffer.pop();

        QString prefix = GetLogPrefix(logEntry.second);
        QString message = QString::fromStdString(logEntry.first);

        if (message.endsWith('\n')) {
            message.chop(1);
        }

        QString fullMessage = prefix + message;
        m_textEdit->appendPlainText(fullMessage);
    }
    if (shouldAutoScroll) {
        scrollBar->setValue(scrollBar->maximum());
    }
}

void LogWidget::SaveLogToFile(const std::string& filePath)
{
    std::ofstream file(filePath, std::ios::out);
    CHECK(file.is_open()) << filePath;
    file << m_textEdit->toPlainText().toUtf8().constData();
}