#ifndef INCLUDE_FS_STAT_H
#define INCLUDE_FS_STAT_H

#include <protura/types.h>

/* mode flags */
#define S_IFMT  0170000
#define S_IFREG 0100000
#define S_IFDIR 0040000
#define S_IFBLK 0060000
#define S_IFCHR 0020000

#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#define S_ISBLK(m) (((m) & S_IFMT) == S_IFBLK)
#define S_ISCHR(m) (((m) & S_IFMT) == S_IFCHR)

#define MODE_TO_DT(mode) (((mode) >> 12) & 15)

#define DT_UNKNOWN 0
#define DT_CHR     2
#define DT_DIR     4
#define DT_BLK     6
#define DT_REG     8

#endif