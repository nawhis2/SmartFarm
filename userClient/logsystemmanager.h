#ifndef LOGSYSTEMMANAGER_H
#define LOGSYSTEMMANAGER_H
#include <QObject>
#include <QDateTime>
#include <QList>
#include <QString>
#include <QStandardItemModel>

struct LogEntry {
    QDateTime  timestamp;
    QString    eventName;
    QString    eventDetail;
};

class LogSystemManager : public QObject {
    Q_OBJECT
public:
    // 싱글턴 접근자
    static LogSystemManager& instance() {
        static LogSystemManager _inst;
        return _inst;
    }

    // 새로운 로그 추가 후 즉시 저장
    void makeAndSaveLogData(const QString& eventName,
                            const QString& eventDetail);
    // 메모리에 있는 로그 전체를 파일에 쓰기
    void saveLogData();
    // 파일에서 로그 읽어 메모리에 로드
    void loadLogData();

    // 메모리 상의 로그 리스트
    const QList<LogEntry>& logs() const { return m_logs; }

    // QStandardItemModel에 "Date, Event, Detail" 칼럼으로 채워주는 유틸
    void populateModel(QStandardItemModel* model);

signals:
    void logAppended(const LogEntry& entry);
    void logsLoaded();

private:
    LogSystemManager();
    ~LogSystemManager() = default;
    LogSystemManager(const LogSystemManager&) = delete;
    LogSystemManager& operator=(const LogSystemManager&) = delete;

    QList<LogEntry> m_logs;
    QString         m_logFilePath;
};

#endif // LOGSYSTEMMANAGER_H
