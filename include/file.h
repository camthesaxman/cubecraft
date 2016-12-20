#ifndef GUARD_FILE_H
#define GUARD_FILE_H

void file_init(void);
void file_enumerate(bool (*callback)(const char *filename));
void file_create(const char *name);
void file_delete(const char *name);

#endif //GUARD_FILE_H
