#ifndef _uri_parser_
#define _uri_parser_
#ifdef  __cplusplus
extern "C" {
#endif
typedef struct {
    char *scheme;               /* mandatory */
    char *host;                 /* mandatory */
    char *port;                 /* optional */
    char *path;                 /* optional */
    char *query;                /* optional */
    char *fragment;             /* optional */
    char *username;             /* optional */
    char *password;             /* optional */
    char *extension;
    char *host_ext;
    char *_uri;                 /* private */
    int _uri_len;               /* private */
    int espField;               /* set as 1 if scheme is 'adc' 'i2s' or 'raw' and when parsed some specific char such as '@' '#' etc.*/
} ParsedUri;

ParsedUri *ParseUri(const char *);
void FreeParsedUri(ParsedUri *);
void ParseUriInfo(ParsedUri *puri);
#ifdef __cplusplus
}
#endif

#endif /* _uri_parser_ */
