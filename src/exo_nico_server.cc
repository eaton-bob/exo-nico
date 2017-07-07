/*  =========================================================================
    exo_nico_server - Actor

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
    exo_nico_server - Actor
@discuss
@end
*/

#include "exo_nico_classes.h"

//  --------------------------------------------------------------------------
//  Actor function


void
s_handle_message (mlm_client_t *client, char *message)
{
    assert (client);
    zmsg_t *reply_message = zmsg_new();
    const char * subject = mlm_client_subject(client);
    const char * mb_exo = "exo-nico";
    const char * mb_ping = "ping";
    const char* mb_dest = mb_exo;
    
    
    if (message == NULL) {
        printf ("Commande : %s Message : NULL \n",mlm_client_command (client));
        zmsg_addstr (reply_message, "NULLVALUE");
    }else if (streq(subject,"ping"))
    {
        printf ("Commande : %s Message Ping \n",mlm_client_command (client));
        zmsg_addstr (reply_message, "GNIP");
        mb_dest = mb_ping;
    }else {
        printf ("Commande : %s Message : %s\n",mlm_client_command (client),message);
        zmsg_addstr (reply_message, message);
    }
    
    //..by the same way it comes
    if (streq (mlm_client_command (client), "MAILBOX DELIVER")) {
        int rv = mlm_client_sendto (client, mlm_client_sender (client), mb_dest, NULL, 1000, &reply_message);
        if (rv != 0) {
            zmsg_destroy (&reply_message);
            zsys_error ("mlm_client_sendto (subject = '%s') failed");
            return;
        }    
    }
    else if (streq (mlm_client_command (client), "STREAM DELIVER"))  {
        int rv = mlm_client_send (client, "example", &reply_message);
        if (rv != 0) {
            zmsg_destroy (&reply_message);
            zsys_error ("mlm_client_send (subject = '%s') failed");
            return;
        }    
    }

    else {
            
        zsys_warning ("Unknown malamute pattern: '%s'. Message subject: '%s', sender: '%s'.",
                    mlm_client_command (client), mlm_client_subject (client), mlm_client_sender (client));
            zstr_free (&message);
    }
}

//  --------------------------------------------------------------------------
// server action
void
exo_nico_server (zsock_t *pipe, void* args)
{
    const char *endpoint = (const char *) args;
    zsys_debug ("endpoint: %s", endpoint);

    mlm_client_t *client = mlm_client_new ();
    if (client == NULL) {
        zsys_error ("mlm_client_new () failed.");
        return;
    }

    int rv = mlm_client_connect (client, endpoint, 1000, "exo-nico");
    if (rv == -1) {
        mlm_client_destroy (&client);
        zsys_error (
                "mlm_client_connect (endpoint = '%s', timeout = '%d', address = '%s') failed.",
                endpoint, 1000, "exo-nico");
        return;
    } 

    rv = mlm_client_set_consumer (client, EXO_NICO_STREAM, ".*");
    if (rv == -1) {
        mlm_client_destroy (&client);
        zsys_error (
                "mlm_client_set_consumer (stream = '%s', pattern = '%s') failed.",
                EXO_NICO_STREAM, ".*");
        return;
    }

    rv = mlm_client_set_producer (client, EXO_NICO_STREAM_RETURN);
    if (rv == -1) {
        mlm_client_destroy (&client);
        zsys_error ("mlm_client_set_consumer (stream = '%s') failed.", EXO_NICO_STREAM_RETURN);
        return;
    } 

    zpoller_t *poller = zpoller_new (pipe, mlm_client_msgpipe (client), NULL);
    zsock_signal (pipe, 0);
    zsys_debug ("actor server ready");

    while (!zsys_interrupted) {

        void *which = zpoller_wait (poller, -1);
        if (which == pipe) {
            zmsg_t *message = zmsg_recv (pipe);
            char *actor_command = zmsg_popstr (message);
            //  $TERM actor command implementation is required by zactor_t interface
            if (streq (actor_command, "$TERM")) {
                zstr_free (&actor_command);
                zmsg_destroy (&message);
                break;
            }
            zstr_free (&actor_command);
            zmsg_destroy (&message);
            continue;
        }

        zmsg_t *message = mlm_client_recv (client);
        if (message == NULL) {
            zsys_debug ("interrupted");
            break;
        }
        
        char* msg = zmsg_popstr (message);

        s_handle_message(client,msg);
        zmsg_destroy(&message);
        zstr_free (&msg);

    }

    mlm_client_destroy (&client);
    zpoller_destroy (&poller);
}

//  --------------------------------------------------------------------------
//  Self test of this class

void
exo_nico_server_test (bool verbose)
{
    printf (" * exo_nico_server: ");

    //  @selftest
    static const char* endpoint = "inproc://exo-nico-server-test";      

     //get client object

    mlm_client_t * streamClient = mlm_client_new();
    assert (streamClient);

    //  Set up broker, fty example server actor and third party actor
    zactor_t *server = zactor_new (mlm_server, (void*)"Malamute");
    zstr_sendx (server, "BIND", endpoint, NULL);
    if (verbose)
        zstr_send (server, "VERBOSE");

    zactor_t *example_server = zactor_new (exo_nico_server, (void *) endpoint);

    printf ("Stream Test\n");

    //  Producer on EXO_NICO_STREAM stream
    int rv = mlm_client_connect (streamClient, endpoint, 1000, "stream");
    assert (rv == 0);
    rv = mlm_client_set_producer (streamClient, EXO_NICO_STREAM);
    assert (rv == 0);

    //  Consumer on EXO_NICO_STREAM_RETURN stream
    rv = mlm_client_set_consumer (streamClient, EXO_NICO_STREAM_RETURN, ".*");
    assert (rv == 0);

    //  Test STREAM deliver:
    //  Create asset message and publish it on EXO_NICO_STREAM stream
    zmsg_t *asset_message = zmsg_new ();
    zmsg_addstr (asset_message, "BWAH");
    assert (asset_message);
    rv = mlm_client_send (streamClient, "exo_nico", &asset_message);
    assert (rv == 0);
    zmsg_t *message = mlm_client_recv (streamClient);
    assert (message);
    char* msgReceive = zmsg_popstr (message);
    assert (streq (msgReceive, "BWAH"));
    zstr_free(&msgReceive);
    zmsg_destroy (&message);
    
    asset_message = zmsg_new ();
    assert (asset_message);
    rv = mlm_client_send (streamClient, "exo_nico", &asset_message);
    assert (rv == 0);
    message = mlm_client_recv (streamClient);
    assert (message);
    msgReceive = zmsg_popstr (message);
    assert (streq (msgReceive, "NULLVALUE"));
    zstr_free(&msgReceive);
    zmsg_destroy (&message);
    
    
    printf ("MailBox Test\n");

    //  Test MAILBOX deliver
    mlm_client_t * mailBoxClient = mlm_client_new();
    assert (mailBoxClient);

    rv = mlm_client_connect (mailBoxClient, endpoint, 1000, "mailbox");
    assert (rv == 0);
    
    //Send a message
    message = zmsg_new ();
    zmsg_addstr (message, "BWAAAAAAH");
    rv = mlm_client_sendto (mailBoxClient, "exo-nico", "exo-nico", NULL, 1000, &message);
    assert (rv == 0);
    message = mlm_client_recv (mailBoxClient);
    assert (message);
    char *ret = zmsg_popstr (message);
    assert (ret!=NULL);
    assert (streq (ret, "BWAAAAAAH"));
    zstr_free (&ret);
    zmsg_destroy (&message);

    //Send a null message 
    message = zmsg_new ();
    rv = mlm_client_sendto (mailBoxClient, "exo-nico", "exo-nico", NULL, 1000, &message);
    assert (rv == 0);
    message = mlm_client_recv (mailBoxClient);
    assert (message);
    char *error = zmsg_popstr (message);
    assert (error!=NULL);
    assert (streq (error, "NULLVALUE"));
    zstr_free (&error);
    zmsg_destroy (&message);
    
    //
    mlm_client_destroy(&mailBoxClient);
    mlm_client_destroy(&streamClient);
    zactor_destroy (&example_server);
    zactor_destroy (&server);
    
    
    
    //  @end
    printf ("OK\n");
}

