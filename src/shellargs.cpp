#include "shellargs.h"

QStringList parseShellArgs(const QString &input) {
    QStringList result;
    QString current;
    bool tokenStarted = false;
    bool inSingle = false;
    bool inDouble = false;

    for (int i = 0; i < input.size(); ++i) {
        const QChar ch = input[i];

        if (inSingle) {
            if (ch == u'\'')
                inSingle = false;
            else
                current += ch;
        } else if (inDouble) {
            if (ch == u'"') {
                inDouble = false;
            } else if (ch == u'\\' && i + 1 < input.size()) {
                // Inside double quotes only \", \\, \$, and \` are special.
                const QChar next = input[i + 1];
                if (next == u'"' || next == u'\\' || next == u'$' || next == u'`') {
                    current += next;
                    ++i;
                } else {
                    current += ch;
                }
            } else {
                current += ch;
            }
        } else {
            if (ch == u'\'') {
                inSingle = true;
                tokenStarted = true;
            } else if (ch == u'"') {
                inDouble = true;
                tokenStarted = true;
            } else if (ch == u'\\' && i + 1 < input.size()) {
                current += input[++i];
                tokenStarted = true;
            } else if (ch.isSpace()) {
                if (tokenStarted) {
                    result << current;
                    current.clear();
                    tokenStarted = false;
                }
            } else {
                current += ch;
                tokenStarted = true;
            }
        }
    }

    if (tokenStarted)
        result << current;

    return result;
}
