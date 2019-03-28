/*
 * ================================
 * eli960@qq.com
 * www.mailhonor.com
 * 2019-03-28
 * ================================
 */


#include <unistd.h>
#include <stdio.h>
#include <dlfcn.h>

typedef void (*show_fn)(int i);

int do_test(int i)
{
    i++;
    void *handler = dlopen("./lib.so", RTLD_LAZY|RTLD_GLOBAL);
    if (!handler) {
        printf("%d can not dlopen lib.so(%m)\n", i);
        return 1;
    }
    show_fn show = (show_fn)dlsym(handler, "show");
    show(i);
    dlclose(handler);

    return i;
}

int main(int argc, char **argv)
{
    dlopen("./lib.so", RTLD_LAZY|RTLD_GLOBAL);

    int i=0;
    for (i=0;i<100;i++) {
        do_test(i);
        sleep(1);
    }
    return 0;
}
