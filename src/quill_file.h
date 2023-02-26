#ifndef _QUILL_FILE_H_
#define _QUILL_FILE_H_

#include "quill.h"
#include "quill_cursor.h"

typedef enum FileCommandType {
  FILE_COMMAN_NONE,
  FILE_COMMAND_INSERT,
  FILE_COMMAND_REMOVE,
  FILE_COMMAND_JOIN_LINES,
  FILE_COMMAND_SPLIT_LINE,
} FileCommandType;

typedef struct FileCommand {
  FileCommandType type;
  Cursor start;
  Cursor end;
  u8 *text;
} FileCommand;

#define FILE_MAX_UNDO_REDO_SIZE 256
typedef struct FileCommandStack {
  FileCommand commands[FILE_MAX_UNDO_REDO_SIZE];
  u32 top;
  u32 size;
} FileCommandStack;

FileCommandStack *file_command_stack_create();
void file_command_stack_destroy(FileCommandStack *stack);
FileCommand *file_command_stack_push(FileCommandStack *stack);
FileCommand *file_command_stack_pop(FileCommandStack *stack);
FileCommand *file_command_stack_top(FileCommandStack *stack);

#define FILE_MAX_NAME_SIZE 256
typedef struct File {

  u8 name[FILE_MAX_NAME_SIZE];
  struct Line **buffer;
  struct Line *line_first_free;
  Cursor cursor_saved;

  FileCommandStack *undo_stack;
  FileCommandStack *redo_stack;

} File;

File *file_create(u8 *filename);
void file_destroy(File *file);

struct Line *file_line_create(File *file);
void file_line_free(File *file, struct Line *line);

File *file_load_from_existing_file(u8 *filename);
void file_insert_new_line(File *file);
void file_insert_new_line_at(File *file, u32 index);
void file_remove_line(File *file);
/* TODO: file_remove_line_at must remove (index + 1) */
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
