// SPDX-License-Identifier: MIT
#include "wet-touch-filter.hpp"

#include <unistd.h>

int main(int argc, char **argv)
{
    if (argc != 2 || geteuid() != 0)
        return 2;
    const QByteArray requested(argv[1]);
    if (requested != "0" && requested != "1")
        return 2;

    const auto path = uq::wetTouchFilterPath();
    if (path.isEmpty())
        return 1;
    if (uq::wetTouchFilterValue(path) == requested)
        return 0;
    QFile file(path);
    const auto value = requested + '\n';
    if (!file.open(QIODevice::WriteOnly | QIODevice::Unbuffered) ||
        file.write(value) != value.size())
        return 1;
    file.close();
    return uq::wetTouchFilterValue(path) == requested ? 0 : 1;
}
