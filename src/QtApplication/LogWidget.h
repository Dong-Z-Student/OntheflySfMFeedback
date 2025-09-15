#pragma once

#include <QWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QGridLayout>
#include <QTimer>
#include <QMutex>
#include <QString>
#include <queue>
#include <string>
#include <glog/logging.h>

class LogWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LogWidget(QWidget *parent = nullptr);
    ~LogWidget();

    void SetMaxLines(int maxLines);
    
    void Append(const std::string& message, google::LogSeverity severity);

    void SaveLogToFile(const std::string& filePath);

private slots:
    void Clear();
    
    void Flush();

private:
    void InitializeUI();
    
    void InitializeLogging();
    
    QString GetLogPrefix(google::LogSeverity severity) const;

private:
    QPlainTextEdit* m_textEdit;
    QPushButton* m_clearButton;
    QGridLayout* m_layout;
    QTimer* m_flushTimer;
    
    QMutex m_bufferMutex;
    std::queue<std::pair<std::string, google::LogSeverity>> m_logBuffer;
    
    int m_maxLines;
    
    class CustomLogSink;
    std::unique_ptr<CustomLogSink> m_logSink;
};

// 自定义日志接收器类
class LogWidget::CustomLogSink : public google::LogSink
{
public:
    explicit CustomLogSink(LogWidget* widget) : m_widget(widget) {}
    
    void send(google::LogSeverity severity, const char* full_filename,
              const char* base_filename, int line,
              const struct ::tm* tm_time,
              const char* message, size_t message_len) override
    {
        if (m_widget) {
            std::string msg(message, message_len);
            m_widget->Append(msg, severity);
        }
    }

private:
    LogWidget* m_widget;
};