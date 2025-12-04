#ifndef _HAL_METADATA_H
#define _HAL_METADATA_H

typedef struct {
    unsigned major;
    unsigned minor;
    unsigned patch;
    const char *version_str;
    const char *git_hash;
    const char *build_date;
} hal_metadata_t;

const hal_metadata_t *hal_get_metadata(void);

#endif /* _HAL_METADATA_H */
