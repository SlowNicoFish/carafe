#pragma once

#include <QStringList>

// Splits a shell-style argument string into a list of tokens, respecting
// single-quoted ('...'), double-quoted ("..."), and backslash-escaped characters.
// Examples:
//   -foo "bar baz"       -> ["-foo", "bar baz"]
//   -x 'hello world'     -> ["-x", "hello world"]
//   -p pass\ word        -> ["-p", "pass word"]
QStringList parseShellArgs(const QString &input);
