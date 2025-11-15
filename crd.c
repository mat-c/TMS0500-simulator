#include <string.h>
#include <stdio.h>
#include "emu.h"

#define MAX_DATA 300

#define WAIT_IN_DATA 1
struct crd {
    int pc;
    unsigned char data[MAX_DATA];
    int flags;
    int flags_delay;
    FILE *file;
};


/*
 * 0 1010 ____ 1000
 *
 *
 * cycle   irg[in]         ext[in]  IO
 * 1       IN CRD          x        x
 * 2       MOV KR,EXT      x        x
 * 3       x               O:data   x #data out ext[3-10]
 *
 * 1       OUT CRD         I:data   x #data in ext[3-10]
 *
 *
 *

 */
 

static int crd_process(void *priv, struct bus *bus)
{
    struct crd *crd = priv;
    if (bus->sstate == 15 && bus->write) {
        if (crd->flags_delay & WAIT_IN_DATA) {
            int data = crd->data[crd->pc];
            bus->ext |= data << 3;
		    LOG ("CRD.rd[%04d]=%02x ", crd->pc, data);
            crd->pc++;
            if (crd->pc >= MAX_DATA)
                crd->pc = 0;
        }
    }
    else if (bus->sstate == 15 && !bus->write) {
        
        /* delay one cycle */
        crd->flags_delay = crd->flags;
        crd->flags = 0;
        /* instruction decode */
        switch (bus->irg) {
            case 0x0A28:
                /* crd data read */
                crd->flags |= WAIT_IN_DATA;
                break;
            case 0x0A38:
            {
                /* crd data write */
                int data = (bus->ext >> 3) & 0xFF;
                crd->data[crd->pc] = data;
                LOG("CRD.wr[%04d]=%02x ", crd->pc, data);
                crd->pc++;
                if (crd->pc >= MAX_DATA)
                    crd->pc = 0;
                break;
            }
            case 0x0A48: /* crd off */
                if (crd->pc)
                        printf("crd read/write %d\n", crd->pc);
                crd_clear_switch();
                if (crd->file && crd->pc) {
                        fseek(crd->file, 0, SEEK_SET);
                        fwrite(crd->data, 1, sizeof(crd->data), crd->file);
                        fflush(crd->file);
                }
                crd->pc = 0;
                break;
            case 0x0A58: /* crd read */
            case 0x0AC8: /* crd write */
                break;
        }
    }
    return 0;
}



int crd_init(struct chip *chip, const char *name)
{
    struct crd *crd;

    crd = malloc(sizeof(*crd));
    if (!crd)
        return -1;

    crd->pc = 0;
    crd->flags = crd->flags_delay = 0;
    memset(crd->data, 0x92, sizeof(crd->data));
    printf("card support\n");
    crd->file = fopen(name, "r+");
    if (!crd->file)
        crd->file = fopen(name, "w+");
    if (crd->file) {
            printf("open crd %s\n", name);
            fseek(crd->file, 0, SEEK_SET);
            fread(crd->data, 1, sizeof(crd->data), crd->file);
    }

    chip->priv = crd;
    chip->process = crd_process;
    return 0;
}
