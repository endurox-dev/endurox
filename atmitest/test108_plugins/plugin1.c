#include <ndebug.h>
#include <ndrstandard.h>

long ndrx_plugin_init(char *provider_name, int provider_name_bufsz)
{
    NDRX_LOG_EARLY(log_info, "Plugin 1");
    return EXSUCCEED;
}
