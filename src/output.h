#ifndef OUTPUT_H
#define OUTPUT_H
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include "main-src.h"
#include "unusedparm.h"
#include "masscan-app.h"

struct Masscan;
struct Output;
enum ApplicationProtocol;

/**
 * Output plugins
 *
 * The various means for writing output are essentially plugins. As new methods
 * are created, we just fill in a structure of function pointers.
 * TODO: this needs to be a loadable DLL, but in the meantime, it's just
 * internal structures.
 */
struct OutputType {
    const char *file_extension;
    void *(*create)(struct Output *out);
    void (*open)(struct Output *out, FILE *fp);
    void (*close)(struct Output *out, FILE *fp);
    void (*status)(struct Output *out, FILE *fp, 
                   time_t timestamp, int status, 
                   unsigned ip, unsigned port, unsigned reason, unsigned ttl);
    void (*banner)(struct Output *out, FILE *fp, 
                   time_t timestamp, unsigned ip, unsigned ip_proto, 
                   unsigned port, enum ApplicationProtocol proto, 
                   const unsigned char *px, unsigned length);
};

/**
 * Masscan creates one "output" structure per thread.
 */
struct Output
{
    char *filename;
    struct Source src[8];
    FILE *fp;
    const struct OutputType *funcs;
    unsigned format;
    
    struct {
        time_t next;
        time_t last;
        unsigned period;
        unsigned offset;
        char *directory;
    } rotate;
    
    unsigned is_banner:1;
    unsigned is_gmt:1; /* --gmt */
    unsigned is_interactive:1; /* echo to command line */
    unsigned is_open_only:1; /*only open ports, not closed ones */
    unsigned is_append:1; /* append to file */
    struct {
        struct {
            uint64_t open;
            uint64_t closed;
            uint64_t banner;
        } tcp;
        struct {
            uint64_t open;
            uint64_t closed;
        } udp;
        struct {
            uint64_t echo;
            uint64_t timestamp;
        } icmp;
		struct {
			uint64_t open;
		} arp;
    } counts;

    struct {
        unsigned ip;
        unsigned port;
        ptrdiff_t fd;
        uint64_t outstanding;
        unsigned state;
    } redis;
    struct {
        char *stylesheet;
    } xml;
};

const char *proto_from_status(unsigned status);
const char *status_string(int x);
const char *reason_string(int x, char *buffer, size_t sizeof_buffer);
const char *normalize_string(const unsigned char *px, size_t length, 
                             char *buf, size_t buf_len);


extern const struct OutputType text_output;
extern const struct OutputType xml_output;
extern const struct OutputType binary_output;
extern const struct OutputType null_output;
extern const struct OutputType redis_output;

/**
 * Creates an "output" object. This is called by the receive thread in order
 * to send "status" information (open/closed ports) and "banners" to either
 * the command-line or to files in specific formats, such as XML or Redis
 * @param masscan
 *      The master configuration.
 * @param thread_index
 *      When there are more than one receive threads, they are differentiated
 *      by this index number.
 * @return
 *      an output object that must eventually be destroyed by output_destroy().
 */
struct Output *
output_create(const struct Masscan *masscan, unsigned thread_index);

void output_destroy(struct Output *output);

void output_report_status(struct Output *output, time_t timestamp, 
    int status, unsigned ip, unsigned port, unsigned reason, unsigned ttl);


typedef void (*OUTPUT_REPORT_BANNER)(
                struct Output *output, time_t timestamp,
                unsigned ip, unsigned ip_proto, unsigned port, 
                unsigned proto,
                const unsigned char *px, unsigned length);

void output_report_banner(
                struct Output *output, 
                time_t timestamp,
                unsigned ip, unsigned ip_proto, unsigned port, 
                unsigned proto,
                const unsigned char *px, unsigned length);

/**
 * Regression tests this unit.
 * @return
 *      0 on success, or positive integer on failure
 */
int
output_selftest(void);




#endif
