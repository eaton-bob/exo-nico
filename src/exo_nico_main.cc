/*  =========================================================================
    exo_nico_main - Binary

    Copyright (C) 2014 - 2017 Eaton                                        
                                                                           
    This program is free software; you can redistribute it and/or modify   
    it under the terms of the GNU General Public License as published by   
    the Free Software Foundation; either version 2 of the License, or      
    (at your option) any later version.                                    
                                                                           
    This program is distributed in the hope that it will be useful,        
    but WITHOUT ANY WARRANTY; without even the implied warranty of         
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          
    GNU General Public License for more details.                           
                                                                           
    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.            
    =========================================================================
*/

/*
@header
    exo_nico_main - Binary
@discuss
@end
*/

#include "exo_nico_classes.h"

int main (int argc, char *argv [])
{
    bool verbose = false;
    const char* config_file = "/etc/default/exo-nico-main.cfg";
    zconfig_t *config = NULL;
     
    int argn;
    for (argn = 1; argn < argc; argn++) {
        if (streq (argv [argn], "--help")
        ||  streq (argv [argn], "-h")) {
            puts ("exo-nico-main [options] ...");
            puts ("  --verbose / -v         verbose test output");
            puts ("  --config / -c          config file [/etc/default/exo-nico-main.cfg]");
            puts ("  --help / -h            this information");
            return 0;
        }
        else
        if (streq (argv [argn], "--verbose")
        ||  streq (argv [argn], "-v")) {
            verbose = true;
        }
        else
        if (streq (argv [argn], "--config")
        ||  streq (argv [argn], "-c")){
            ++argn;
            if (argn < argc) {
                config_file = argv [argn];
            }
        }
        else {
            printf ("Unknown option: %s\n", argv [argn]);
            return 1;
        }
    }
    //  Insert main code here
    if (verbose)
        zsys_info ("exo_nico_main - Binary");
    
    //get paramters from config file
     config = zconfig_load (config_file);
     if (!config) {
        zsys_error ("Failed to load config file %s : %m", config_file);
        exit (EXIT_FAILURE);
     }
        
    char *endpoint = zconfig_get(config,"malamute/endpoint","ipc://@/malamute");

    zsys_info ("exo-nico starting");
    zactor_t *server = zactor_new (exo_nico_server, (void *) endpoint);
    zactor_t *client = zactor_new (exo_nico_client, (void *) endpoint);

    //client to send order to client
    mlm_client_t* clientMain = mlm_client_new();
    int rv = mlm_client_connect (clientMain, endpoint, 1000, "main");
    if (rv == -1) {
        mlm_client_destroy (&clientMain);
        zsys_error (
                "mlm_client_connect (endpoint = '%s', timeout = '%d', address = '%s') failed.",
                endpoint, 1000, "main");
        return EXIT_FAILURE;
    } 

    //  Accept and print any messconstage back from server
    //  copy from src/malamute.c under MPL license
    while (true) {
        char *msg_srv = zstr_recv (server);
        char *msg_clt = zstr_recv (client);
        if (msg_srv) {
            puts (msg_srv);
            zstr_free (&msg_srv);
        }

        if (msg_clt) {
            puts (msg_clt);
            zstr_free (&msg_clt);
        }
        
        if (!msg_srv || !msg_clt){
            puts ("interrupted");
            break;
        }
        
        
        zmsg_t* message = zmsg_new ();
        zmsg_addstr (message, "ping");
        rv = mlm_client_sendto (clientMain, "main", "ping", NULL, 1000, &message);
        
        sleep(2);
    }
    
    zactor_destroy (&server);
    mlm_client_destroy(&clientMain);
    return EXIT_SUCCESS;
}
