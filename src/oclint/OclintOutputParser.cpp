#include "OclintOutputParser.h"
#include "OclintConstants.h"

#include <projectexplorer/taskhub.h>

#include <QRegularExpression>

using namespace ProjectExplorer;

namespace QtcUtilities {
  namespace Internal {
    namespace Oclint {

      ToolOutputParser::ToolOutputParser (QObject *parent) : QObject (parent) {
      }

      void ToolOutputParser::setCheckingFiles (const QStringList &files) {
        clearTasks (files);
      }

      void ToolOutputParser::parseStandardError (const QByteArray &data) {
        parseOutput (data);
      }

      void ToolOutputParser::parseStandardOutput (const QByteArray &data) {
        parseOutput (data);
      }

      void ToolOutputParser::parseOutput (const QByteArray &data) {
        enum {
          Full, File, Line, Column, Summary, Type, Priority, Message
        };
        QRegularExpression re (QLatin1String (R"((.+):(\d+):(\d+): (.+) \[(\w+)\|(\w+)\] (.+))"));
        QMap<QString, Task::TaskType> types;
        types.insert (QLatin1String ("P3"), Task::Warning);
        types.insert (QLatin1String ("P2"), Task::Warning);
        types.insert (QLatin1String ("P1"), Task::Error);

        auto lines = data.split ('\n');
        for (const auto &rawLine: lines) {
          QRegularExpressionMatch match = re.match (QString::fromUtf8 (rawLine));
          if (!match.hasMatch ()) {
            continue;
          }

          auto type = types.value (match.captured (Priority), Task::Unknown);
          auto fileString = match.captured (File);
          auto file = Utils::FileName::fromString (fileString);
          auto line = match.captured (Line).toInt ();
          auto description = QString ("OCLint: %2 (%3): %4").arg (
            match.captured (Summary), match.captured (Type), match.captured (Message));
          Task task (type, description, file, line, TASK_CATEGORY_ID);
          TaskHub::addTask (task);
          taskIdsPerFile_[fileString].insert (task.taskId);
        }
      }

      void ToolOutputParser::clearTasks (const QStringList &files) {
        for (const auto &file: files) {
          for (auto id: taskIdsPerFile_[file]) {
            Task task;
            task.taskId = id;
            TaskHub::removeTask (task);
          }
          taskIdsPerFile_.remove (file);
        }
      }

    } // namespace Oclint
  } // namespace Internal
} // namespace QtcUtilities
