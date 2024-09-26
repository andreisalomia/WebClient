#ifndef _REQUESTS_
#define _REQUESTS_

// computes and returns a GET request string (query_params
// and cookies can be set to NULL if not needed)
char *compute_get_request(const char *host, char *url, char *query_params,
							char *cookie, char *token);

// computes and returns a POST request string (cookies can be NULL if not needed)
char *compute_post_request(const char *host, const char *url, const char *content_type, const char *body_data,
						 char *cookie, char *token);


char *compute_delete_request(const char *host, char *url, char *query_params, char *cookie, char *token);
#endif
