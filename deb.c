/*
 * Debian package gateway for the ESP Package Manager (EPM).
 *
 * Copyright 2020 by Jim Jagielski
 * Copyright 1999-2017 by Michael R Sweet
 * Copyright 1999-2010 by Easy Software Products.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Include necessary headers...
 */

#include "epm.h"

/*
 * Local functions...
 */

/*
 * 'add_size()' - Append Installed-Size tag to DEBIAN/control file
 *                Used for AOO packages
 */
static int                      /* O - 0 = success, 1 = fail */
add_size(FILE *fpControl,       /* Control file stream */
         const char *directory) /* Directory containing all files to package */
{
    FILE *fp;
    char command[1024];

    snprintf(command, sizeof(command), "du -k -s %s", directory);
    fp = popen(command, "r");
    if (NULL != fp) {
        char size[1024];
        fscanf(fp, "%s .", size);
        fprintf(fpControl, "Installed-Size: %s\n", size);
        return pclose(fp);
    }
    return 1;
}

static int make_subpackage(const char *prodname, const char *directory,
                           const char *platname, dist_t *dist, struct utsname *platform,
                           const char *subpackage);

/*
 * 'make_deb()' - Make a Debian software distribution package.
 */

int                                /* O - 0 = success, 1 = fail */
make_deb(const char *prodname,     /* I - Product short name */
         const char *directory,    /* I - Directory for distribution files */
         const char *platname,     /* I - Platform name */
         dist_t *dist,             /* I - Distribution information */
         struct utsname *platform) /* I - Platform information */
{
    int i;              /* Looping var */
    tarf_t *tarfile;    /* Distribution tar file */
    char name[1024],    /* Full product name */
        filename[1024]; /* File to archive */
    char *sep;

    /*
     * OpenOffice builds have traditionally used '_' instead of '-' for versioning
     */
    sep = (AooMode ? "_" : "-");

    if (AooMode) {
        /*
         * Use debian default naming scheme
         */
        if (!strcmp(platname, "intel"))
#ifdef __FreeBSD_kernel__
            platname = "kfreebsd-i386";
#else
            platname = "i386";
#endif
        else if (!strcmp(platname, "x86_64"))
#ifdef __FreeBSD_kernel__
            platname = "kfreebsd-amd64";
#else
            platname = "amd64";
#endif
        else if (!strcmp(platname, "ppc"))
            platname = "powerpc";

    } else {
        /* Debian packages use "amd64" instead of "x86_64" for the architecture... */
        if (!strcmp(platname, "x86_64"))
            platname = "amd64";
    }

    if (make_subpackage(prodname, directory, platname, dist, platform, NULL))
        return (1);

    for (i = 0; i < dist->num_subpackages; i++)
        if (make_subpackage(prodname, directory, platname, dist, platform,
                            dist->subpackages[i]))
            return (1);

    /*
     * Build a compressed tar file to hold all of the subpackages...
     */

    if (dist->num_subpackages) {
        /*
         * Figure out the full name of the distribution...
         */
        if (dist->release[0])
            snprintf(name, sizeof(name), "%s%s%s%s%s", prodname, sep, dist->version, sep,
                     dist->release);
        else
            snprintf(name, sizeof(name), "%s%s%s", prodname, sep, dist->version);

        if (platname[0]) {
            strlcat(name, sep, sizeof(name));
            strlcat(name, platname, sizeof(name));
        }

        /*
         * Create a compressed tar file...
         */

        snprintf(filename, sizeof(filename), "%s/%s.deb.tgz", directory, name);

        if ((tarfile = tar_open(filename, 1)) == NULL)
            return (1);

        /*
         * Archive the main package and subpackages...
         */

        if (tar_package(tarfile, "deb", prodname, directory, platname, dist, NULL)) {
            tar_close(tarfile);
            return (1);
        }

        for (i = 0; i < dist->num_subpackages; i++) {
            if (tar_package(tarfile, "deb", prodname, directory, platname, dist,
                            dist->subpackages[i])) {
                tar_close(tarfile);
                return (1);
            }
        }

        tar_close(tarfile);
    }

    /*
     * Remove temporary files...
     */

    if (!KeepFiles && dist->num_subpackages) {
        if (Verbosity)
            puts("Removing temporary distribution files...");

        /*
         * Remove .deb files since they are now in a .tgz file...
         */

        unlink_package("deb", prodname, directory, platname, dist, NULL);

        for (i = 0; i < dist->num_subpackages; i++)
            unlink_package("deb", prodname, directory, platname, dist,
                           dist->subpackages[i]);
    }

    return (0);
}

/*
 * 'make_subpackage()' - Make a subpackage...
 */

static int                                /* O - 0 = success, 1 = fail */
make_subpackage(const char *prodname,     /* I - Product short name */
                const char *directory,    /* I - Directory for distribution files */
                const char *platname,     /* I - Platform name */
                dist_t *dist,             /* I - Distribution information */
                struct utsname *platform, /* I - Platform information */
                const char *subpackage)   /* I - Subpackage */
{
    int i, j;                      /* Looping vars */
    const char *header;            /* Dependency header string */
    FILE *fp;                      /* Control file */
    char prodfull[255],            /* Full name of product */
        name[1024],                /* Full product name */
        filename[1024];            /* Destination filename */
    command_t *c;                  /* Current command */
    depend_t *d;                   /* Current dependency */
    file_t *file;                  /* Current distribution file */
    struct passwd *pwd;            /* Pointer to user record */
    struct group *grp;             /* Pointer to group record */
    static const char *depends[] = /* Dependency names */
        {"Depends:", "Conflicts:", "Replaces:", "Provides:"};
    char *sep;

    /*
     * OpenOffice builds have traditionally used '_' instead of '-' for versioning
     */
    sep = (AooMode ? "_" : "-");
    /*
     * Figure out the full name of the distribution...
     */

    if (subpackage)
        snprintf(prodfull, sizeof(prodfull), "%s%s%s", prodname, sep, subpackage);
    else
        strlcpy(prodfull, prodname, sizeof(prodfull));

    /*
     * Then the subdirectory name...
     */

    if (dist->release[0])
        snprintf(name, sizeof(name), "%s%s%s%s%s", prodfull, sep, dist->version, sep,
                 dist->release);
    else
        snprintf(name, sizeof(name), "%s%s%s", prodfull, sep, dist->version);

    if (platname[0]) {
        strlcat(name, sep, sizeof(name));
        strlcat(name, platname, sizeof(name));
    }

    if (Verbosity)
        printf("Creating Debian %s distribution...\n", name);

    /*
     * Write the control file for DPKG...
     */

    if (Verbosity)
        puts("Creating control file...");

    snprintf(filename, sizeof(filename), "%s/%s", directory, name);
    mkdir(filename, 0777);
    strlcat(filename, "/DEBIAN", sizeof(filename));
    mkdir(filename, 0777);
    chmod(filename, 0755);

    strlcat(filename, "/control", sizeof(filename));

    if ((fp = fopen(filename, "w")) == NULL) {
        fprintf(stderr, "epm: Unable to create control file \"%s\": %s\n", filename,
                strerror(errno));
        return (1);
    }

    fprintf(fp, "Package: %s\n", prodfull);
    if (dist->release[0])
        fprintf(fp, "Version: %s-%s\n", dist->version, dist->release);
    else
        fprintf(fp, "Version: %s\n", dist->version);
    fprintf(fp, "Maintainer: %s\n", dist->vendor);

    /*
     * The Architecture attribute needs to match the uname info
     * (which we change in get_platform to a common name)
     */

    if (!strcmp(platform->machine, "intel"))
#ifdef __FreeBSD_kernel__
        fputs("Architecture: kfreebsd-i386\n", fp);
#else
        fputs("Architecture: i386\n", fp);
#endif
    else if (!strcmp(platform->machine, "x86_64"))
#ifdef __FreeBSD_kernel__
        fputs("Architecture: kfreebsd-amd64\n", fp);
#else
        fputs("Architecture: amd64\n", fp);
#endif
    else if (!strcmp(platform->machine, "ppc"))
        fputs("Architecture: powerpc\n", fp);
    else
        fprintf(fp, "Architecture: %s\n", platform->machine);

    fprintf(fp, "Description: %s\n", dist->product);
    fprintf(fp, " Copyright: %s\n", dist->copyright);
    for (i = 0; i < dist->num_descriptions; i++)
        if (dist->descriptions[i].subpackage == subpackage)
            fprintf(fp, " %s\n", dist->descriptions[i].description);

    for (j = DEPEND_REQUIRES; j <= DEPEND_PROVIDES; j++) {
        for (i = dist->num_depends, d = dist->depends; i > 0; i--, d++)
            if (d->type == j && d->subpackage == subpackage)
                break;

        if (i) {
            for (header = depends[j]; i > 0; i--, d++, header = ",")
                if (d->type == j && d->subpackage == subpackage) {
                    if (!strcmp(d->product, "_self"))
                        fprintf(fp, "%s %s", header, prodname);
                    else
                        fprintf(fp, "%s %s", header, d->product);

                    if (d->vernumber[0] == 0) {
                        if (d->vernumber[1] < INT_MAX)
                            fprintf(fp, " (<= %s)", d->version[1]);
                    } else {
                        if (d->vernumber[1] < INT_MAX)
                            fprintf(fp, " (>= %s), %s (<= %s)", d->version[0], d->product,
                                    d->version[1]);
                        else
                            fprintf(fp, " (>= %s)", d->version[0]);
                    }
                }

            putc('\n', fp);
        }
    }

    for (i = dist->num_commands, c = dist->commands; i > 0; i--, c++)
        if (c->type == COMMAND_LITERAL && c->subpackage == subpackage &&
            !strcmp(c->section, "control"))
            break;

    if (i > 0) {
        for (; i > 0; i--, c++)
            if (c->type == COMMAND_LITERAL && c->subpackage == subpackage &&
                !strcmp(c->section, "control"))
                fprintf(fp, "%s\n", c->command);
    }

    fclose(fp);

    /*
     * Write the templates file for DPKG...
     */

    for (i = dist->num_commands, c = dist->commands; i > 0; i--, c++)
        if (c->type == COMMAND_LITERAL && c->subpackage == subpackage &&
            !strcmp(c->section, "templates"))
            break;

    if (i) {
        if (Verbosity)
            puts("Creating templates file...");

        snprintf(filename, sizeof(filename), "%s/%s/DEBIAN/templates", directory, name);

        if ((fp = fopen(filename, "w")) == NULL) {
            fprintf(stderr, "epm: Unable to create template file \"%s\": %s\n", filename,
                    strerror(errno));
            return (1);
        }

        fchmod(fileno(fp), 0644);

        for (; i > 0; i--, c++)
            if (c->type == COMMAND_LITERAL && c->subpackage == subpackage &&
                !strcmp(c->section, "templates"))
                fprintf(fp, "%s\n", c->command);

        fclose(fp);
    }

    /*
     * Write the preinst file for DPKG...
     */

    for (i = dist->num_commands, c = dist->commands; i > 0; i--, c++)
        if (c->type == COMMAND_PRE_INSTALL && c->subpackage == subpackage)
            break;

    if (i) {
        if (Verbosity)
            puts("Creating preinst script...");

        snprintf(filename, sizeof(filename), "%s/%s/DEBIAN/preinst", directory, name);

        if ((fp = fopen(filename, "w")) == NULL) {
            fprintf(stderr, "epm: Unable to create script file \"%s\": %s\n", filename,
                    strerror(errno));
            return (1);
        }

        fchmod(fileno(fp), 0755);

        fputs("#!/bin/sh\n", fp);
        fputs("# " EPM_VERSION "\n", fp);

        for (; i > 0; i--, c++)
            if (c->type == COMMAND_PRE_INSTALL && c->subpackage == subpackage)
                fprintf(fp, "%s\n", c->command);

        fclose(fp);
    }

    /*
     * Write the postinst file for DPKG...
     */

    for (i = dist->num_commands, c = dist->commands; i > 0; i--, c++)
        if (c->type == COMMAND_POST_INSTALL && c->subpackage == subpackage)
            break;

    if (!i)
        for (i = dist->num_files, file = dist->files; i > 0; i--, file++)
            if (tolower(file->type) == 'i' && file->subpackage == subpackage)
                break;

    if (i) {
        if (Verbosity)
            puts("Creating postinst script...");

        snprintf(filename, sizeof(filename), "%s/%s/DEBIAN/postinst", directory, name);

        if ((fp = fopen(filename, "w")) == NULL) {
            fprintf(stderr, "epm: Unable to create script file \"%s\": %s\n", filename,
                    strerror(errno));
            return (1);
        }

        fchmod(fileno(fp), 0755);

        fputs("#!/bin/sh\n", fp);
        fputs("# " EPM_VERSION "\n", fp);

        for (i = dist->num_commands, c = dist->commands; i > 0; i--, c++)
            if (c->type == COMMAND_POST_INSTALL && c->subpackage == subpackage)
                fprintf(fp, "%s\n", c->command);

        for (i = dist->num_files, file = dist->files; i > 0; i--, file++)
            if (tolower(file->type) == 'i' && file->subpackage == subpackage) {
                /*
                 * Debian's update-rc.d has changed over the years; current practice is
                 * to let update-rc.d choose the runlevels and ordering...
                 */

                fprintf(fp, "update-rc.d %s defaults\n", file->dst);
                fprintf(fp, "/etc/init.d/%s start\n", file->dst);
            }

        fclose(fp);
    }

    /*
     * Write the prerm file for DPKG...
     */

    for (i = dist->num_commands, c = dist->commands; i > 0; i--, c++)
        if (c->type == COMMAND_PRE_REMOVE && c->subpackage == subpackage)
            break;

    if (!i)
        for (i = dist->num_files, file = dist->files; i > 0; i--, file++)
            if (tolower(file->type) == 'i' && file->subpackage == subpackage)
                break;

    if (i) {
        if (Verbosity)
            puts("Creating prerm script...");

        snprintf(filename, sizeof(filename), "%s/%s/DEBIAN/prerm", directory, name);

        if ((fp = fopen(filename, "w")) == NULL) {
            fprintf(stderr, "epm: Unable to create script file \"%s\": %s\n", filename,
                    strerror(errno));
            return (1);
        }

        fchmod(fileno(fp), 0755);

        fputs("#!/bin/sh\n", fp);
        fputs("# " EPM_VERSION "\n", fp);

        for (i = dist->num_commands, c = dist->commands; i > 0; i--, c++)
            if (c->type == COMMAND_PRE_REMOVE && c->subpackage == subpackage)
                fprintf(fp, "%s\n", c->command);

        for (i = dist->num_files, file = dist->files; i > 0; i--, file++)
            if (tolower(file->type) == 'i' && file->subpackage == subpackage)
                fprintf(fp, "/etc/init.d/%s stop\n", file->dst);

        fclose(fp);
    }

    /*
     * Write the postrm file for DPKG...
     */

    for (i = dist->num_commands, c = dist->commands; i > 0; i--, c++)
        if (c->type == COMMAND_POST_REMOVE && c->subpackage == subpackage)
            break;

    if (!i)
        for (i = dist->num_files, file = dist->files; i > 0; i--, file++)
            if (tolower(file->type) == 'i' && file->subpackage == subpackage)
                break;

    if (i) {
        if (Verbosity)
            puts("Creating postrm script...");

        snprintf(filename, sizeof(filename), "%s/%s/DEBIAN/postrm", directory, name);

        if ((fp = fopen(filename, "w")) == NULL) {
            fprintf(stderr, "epm: Unable to create script file \"%s\": %s\n", filename,
                    strerror(errno));
            return (1);
        }

        fchmod(fileno(fp), 0755);

        fputs("#!/bin/sh\n", fp);
        fputs("# " EPM_VERSION "\n", fp);

        for (i = dist->num_commands, c = dist->commands; i > 0; i--, c++)
            if (c->type == COMMAND_POST_REMOVE && c->subpackage == subpackage)
                fprintf(fp, "%s\n", c->command);

        for (i = dist->num_files, file = dist->files; i > 0; i--, file++)
            if (tolower(file->type) == 'i' && file->subpackage == subpackage) {
                fputs("if [ purge = \"$1\" ]; then\n", fp);
                fprintf(fp, "	update-rc.d %s remove\n", file->dst);
                fputs("fi\n", fp);
            }

        fclose(fp);
    }

    /*
     * Write the conffiles file for DPKG...
     */

    if (Verbosity)
        puts("Creating conffiles...");

    snprintf(filename, sizeof(filename), "%s/%s/DEBIAN/conffiles", directory, name);

    if ((fp = fopen(filename, "w")) == NULL) {
        fprintf(stderr, "epm: Unable to create script file \"%s\": %s\n", filename,
                strerror(errno));
        return (1);
    }

    for (i = dist->num_files, file = dist->files; i > 0; i--, file++)
        if (tolower(file->type) == 'c' && file->subpackage == subpackage)
            fprintf(fp, "%s\n", file->dst);
        else if (tolower(file->type) == 'i' && file->subpackage == subpackage)
            fprintf(fp, "/etc/init.d/%s\n", file->dst);

    fclose(fp);

    if (AooMode) {
        /*
         * Calculate and append Installed-Size to DEBIAN/control
         */

        if (Verbosity)
            puts("Calculating Installed-Size...");

        snprintf(filename, sizeof(filename), "%s/%s/DEBIAN/control", directory, name);
        if ((fp = fopen(filename, "a")) == NULL) {
            fprintf(stderr, "epm: Unable to Installed-Size to file \"%s\" - %s\n",
                    filename, strerror(errno));
            return (1);
        }

        snprintf(filename, sizeof(filename), "%s/%s", directory, name);
        add_size(fp, filename);
        fclose(fp);
    }

    /*
     * Copy the files over...
     */

    if (Verbosity)
        puts("Copying temporary distribution files...");

    for (i = dist->num_files, file = dist->files; i > 0; i--, file++) {
        if (file->subpackage != subpackage)
            continue;

        /*
         * Find the username and groupname IDs...
         */

        pwd = getpwnam(file->user);
        grp = getgrnam(file->group);

        endpwent();
        endgrent();

        /*
         * Copy the file or make the directory or make the symlink as needed...
         */

        switch (tolower(file->type)) {
        case 'c':
        case 'f':
            snprintf(filename, sizeof(filename), "%s/%s%s", directory, name, file->dst);

            if (Verbosity > 1)
                printf("%s -> %s...\n", file->src, filename);

            if (copy_file(filename, file->src, file->mode, pwd ? pwd->pw_uid : 0,
                          grp ? grp->gr_gid : 0))
                return (1);
            break;
        case 'i':
            snprintf(filename, sizeof(filename), "%s/%s/etc/init.d/%s", directory, name,
                     file->dst);

            if (Verbosity > 1)
                printf("%s -> %s...\n", file->src, filename);

            if (copy_file(filename, file->src, file->mode, pwd ? pwd->pw_uid : 0,
                          grp ? grp->gr_gid : 0))
                return (1);
            break;
        case 'd':
            snprintf(filename, sizeof(filename), "%s/%s%s", directory, name, file->dst);

            if (Verbosity > 1)
                printf("Directory %s...\n", filename);

            make_directory(filename, file->mode, pwd ? pwd->pw_uid : 0,
                           grp ? grp->gr_gid : 0);
            break;
        case 'l':
            snprintf(filename, sizeof(filename), "%s/%s%s", directory, name, file->dst);

            if (Verbosity > 1)
                printf("%s -> %s...\n", file->src, filename);

            make_link(filename, file->src);
            break;
        }
    }

    /*
     * Build the distribution from the spec file...
     */

    if (Verbosity)
        printf("Building Debian %s binary distribution...\n", name);

    if (geteuid() && !run_command(NULL, "fakeroot --version")) {
        if (run_command(directory, "fakeroot dpkg --build %s", name))
            return (1);
    } else if (run_command(directory, "dpkg --build %s", name))
        return (1);

    /*
     * Remove temporary files...
     */

    if (!KeepFiles) {
        if (Verbosity)
            printf("Removing temporary %s distribution files...\n", name);

        snprintf(filename, sizeof(filename), "%s/%s", directory, name);
        unlink_directory(filename);
    }

    return (0);
}
