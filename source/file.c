#include "global.h"
#include "file.h"

//TODO: Use actual error handling rather than assert

const char *savePath = "/apps/cubecraft/worlds";

static bool can_open_root_dir(void)
{
    DIR *rootDir = opendir("/");
    if (rootDir != NULL)
    {
        closedir(rootDir);
        return true;
    }
    else
    {
        return false;
    }
}

static void ensure_worlds_dir(void)
{
    DIR *saveDir = opendir(savePath);
    
    if (saveDir != NULL)
    {
        closedir(saveDir);
    }
    else
    {
        //Directory doesn't exist. Create it.
        mkdir(savePath, 0777); //This 0777 is in octal
        
        //Make sure we can open it
        saveDir = opendir(savePath);
        assert(saveDir != NULL);
        closedir(saveDir);
    }
}

void file_init(void)
{
    assert(fatInitDefault());
    assert(can_open_root_dir());
    ensure_worlds_dir();
}

void file_enumerate(bool (*callback)(const char *filename))
{
    DIR *saveDir = opendir(savePath);
    struct dirent *d;
    
    assert(saveDir != NULL);
    while ((d = readdir(saveDir)) != NULL)
    {
        if (d->d_name[0] != '.')
        {
            if (!callback(d->d_name))
                break;
        }
    }
    closedir(saveDir);
}

void file_create(const char *name)
{
    char *path = malloc(strlen(savePath) + 1 + strlen(name) + 1);
    FILE *file;
    
    strcpy(path, savePath);
    strcat(path, "/");
    strcat(path, name);
    
    file = fopen(path, "w");
    assert(file != NULL);
    fclose(file);
}

void file_delete(const char *name)
{
    char *path = malloc(strlen(savePath) + 1 + strlen(name) + 1);
    
    strcpy(path, savePath);
    strcat(path, "/");
    strcat(path, name);
    
    remove(path);
}
