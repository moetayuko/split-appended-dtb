/*
 * Copyright (C) 2016 dianlujitao <dianlujitao@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with This program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

/*
From scripts/dtc/libfdt/fdt.h
#define FDT_MAGIC	0xd00dfeed
*/
const uint8_t fdt_magic[] = {0xd0, 0x0d, 0xfe, 0xed};

void *memmem(const void *haystack, size_t n, const void *needle, size_t m)
{
    if (m > n || !m || !n)
        return NULL;

    if (__builtin_expect((m > 1), 1)) {
        const unsigned char*  y = (const unsigned char*) haystack;
        const unsigned char*  x = (const unsigned char*) needle;
        size_t                j = 0;
        size_t                k = 1, l = 2;

        if (x[0] == x[1]) {
            k = 2;
            l = 1;
        }
        while (j <= n-m) {
            if (x[1] != y[j+1]) {
                j += k;
            } else {
                if (!memcmp(x+2, y+j+2, m-2) && x[0] == y[j])
                    return (void*) &y[j];
                j += l;
            }
        }
    } else {
        /* degenerate case */
        return memchr(haystack, ((unsigned char*)needle)[0], n);
    }
    return NULL;
}

int dump_file(FILE *file, uint8_t *head, unsigned long long len)
{
    return fwrite(head, len, 1, file);
}

int split(char *kernel_image)
{
    FILE *fp = NULL;
    int dtb_count = 0, complete = 0, rc = 0;
    unsigned long long kernel_size = 0, len = 0;
    char outfile[20];
    uint8_t *kernel, *kernel_end, *dtb_head, *dtb_next;

    fp = fopen(kernel_image, "rb");
    if (fp == NULL)
    {
        printf("Open %s failed!\n", kernel_image);
        return 1;
    }

    // Get kernel size
    fseek(fp, 0, SEEK_END);
    kernel_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    kernel = (uint8_t *)malloc(kernel_size * sizeof(uint8_t));
    if (kernel == NULL)
    {
        printf("Failed to allocate memory!\n");
        goto err;
    }
    kernel_end = kernel + kernel_size;

    rc = fread(kernel, kernel_size * sizeof(uint8_t), 1, fp);
    fclose(fp);
    if (!rc)
    {
        printf("Failed to read kernel image!\n");
        goto err;
    }

    // Find the first dtb
    dtb_head = memmem(kernel, kernel_size, fdt_magic, sizeof(fdt_magic));
    if (dtb_head == NULL)
    {
        printf("ERROR: Appended Device Tree Blob not found!\n");
        goto err;
    }

    len = dtb_head - kernel;
    fp = fopen("kernel", "wb");
    rc = dump_file(fp, kernel, len);
    fclose(fp);
    if (!rc)
    {
        printf("Failed to dump standalone kernel image!\n");
        goto err;
    }

    while (!complete)
    {
        // Find the next dtbs
        len = kernel_end - dtb_head - sizeof(fdt_magic);
        dtb_next = memmem(dtb_head + sizeof(fdt_magic), len, fdt_magic, sizeof(fdt_magic));
        if (dtb_next == NULL)
        {
            len = kernel_end - dtb_head;
            complete = 1;
        }
        else
            len = dtb_next - dtb_head;

        // Dump found dtbs
        sprintf(outfile, "dtbdump_%d.dtb", ++dtb_count);
        fp = fopen(outfile, "wb");
        rc = dump_file(fp, dtb_head, len);
        fclose(fp);
        if (!rc)
            printf("Failed to dump %s!\n", outfile);

        dtb_head = dtb_next;
    }

    printf("Found %d appended dtbs, please check the output.\n", dtb_count);

    free(kernel);
    return 0;

err:
    free(kernel);
    return 1;
}

int main(int argc, char *argv[])
{
    // Handle alternative invocations
    char* command = argv[0];
    char* stripped = strrchr(argv[0], '/');
    if (stripped)
        command = stripped + 1;

    if (argc == 1)
    {
        printf("Usage:\n\t%s Image-dtb\n", command);
        return 0;
    }

    char *image = argv[1];
    int rc;

    rc = split(image);

    return rc;
}
