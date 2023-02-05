#ifndef _QUILL_FILE_H_
#define _QUILL_FILE_H_

#include "quill.h"

#define FILE_MAX_NAME_SIZE 256
typedef struct File {
  u8 name[FILE_MAX_NAME_SIZE];
  struct Line **buffer;
  struct Line *line_first_free;
} File;

File *file_create(u8 *filename);
void file_destroy(File *file);

struct Line *file_line_create(File *file);
void file_line_free(File *file, struct Line *line);

File *file_load_from_existing_file(u8 *filename);
void file_insert_new_line(File *file);
void file_insert_new_line_at(File *file, u32 index);
void file_remove_line(File *file);
void file_remove_line_at(File *file, u32 index);
void file_print(File *file);
struct Line *file_get_line_at(File *file, u32 index);
u32 file_line_count(File *file);

#define FOLDER_MAX_NAME_SIZE 256
typedef struct Folder {
  u8 name[FOLDER_MAX_NAME_SIZE];
  File **files;
  struct Folder **folders;
} Folder;

Folder *folder_create(u8 *name);
Folder *folder_load(u8 *folderpath);
void folder_destroy(Folder *folder);

void folder_add_file(Folder *folder, File *file);
void folder_add_folder(Folder *parent, Folder *child);

#endif /* _QUILL_FILE_H_ */
