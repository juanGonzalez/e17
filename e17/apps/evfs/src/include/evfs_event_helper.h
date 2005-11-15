#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

void evfs_file_monitor_event_create(evfs_client* client, int type, const char* pathi, const char* plugin);
void evfs_stat_event_create(evfs_client* client, evfs_command* command, struct stat* stat_obj);
void evfs_list_dir_event_create(evfs_client* client, evfs_command* command, Ecore_List* files);
void evfs_file_progress_event_create(evfs_client* client, evfs_command* command, double progress);
