/*  =========================================================================
    exo_nico_client - Actor

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
    exo_nico_client - Actor
@discuss
@end
*/

#include "exo_nico_classes.h"

//  Structure of our class
struct _exo_nico_client_t {
    //  Declare class properties here
    mlm_client_t *client_Stream;
    mlm_client_t *client_Mailbox;
    mlm_client_t *client_Main;
};

//  --------------------------------------------------------------------------
//  Create a new exo_nico_client

exo_nico_client_t *
exo_nico_client_new (void)
{
    exo_nico_client_t *self = (exo_nico_client_t *) zmalloc (sizeof (exo_nico_client_t));
    assert (self);
    self->client_Stream =  mlm_client_new ();
    self->client_Mailbox =  mlm_client_new ();
    self->client_Main =  mlm_client_new ();
    //  Initialize class properties here
    return self;
}


//  --------------------------------------------------------------------------
//  Destroy the exo_nico_client

void
exo_nico_client_destroy (exo_nico_client_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        exo_nico_client_t *self = *self_p;
        //  Free class properties here
        mlm_client_destroy(&self->client_Stream);
        mlm_client_destroy(&self->client_Mailbox);
        mlm_client_destroy(&self->client_Main);
        //  Free object itself
        free (self);
        *self_p = NULL;
    }
}

///////////////////////////////
//Actor action

void
exo_nico_client(zsock_t *pipe, void* args)
{
    //Create client object
    exo_nico_client_t *self = exo_nico_client_new ();
    const char *endpoint = (const char *) args;
    
    //client connected to main program
    int rv = mlm_client_connect (self->client_Main, endpoint, 1000, "main");
    if (rv == -1) {
        mlm_client_destroy (&self->client_Main);
        zsys_error (
                "mlm_client_connect (endpoint = '%s', timeout = '%d', address = '%s') failed.",
                endpoint, 1000, "main");
        return;
    } 

    //client connected to mailbox server
    rv = mlm_client_connect (self->client_Mailbox, endpoint, 1000, "ping");
    if (rv == -1) {
        mlm_client_destroy (&self->client_Mailbox);
        zsys_error (
                "mlm_client_connect (endpoint = '%s', timeout = '%d', address = '%s') failed.",
                endpoint, 1000, "ping");
        return;
    } 

    //client connected to stream server
    rv = mlm_client_set_consumer (self->client_Stream, EXO_NICO_STREAM_RETURN, ".*");
    if (rv == -1) {
        mlm_client_destroy (&self->client_Stream);
        zsys_error (
                "mlm_client_set_consumer (stream = '%s', pattern = '%s') failed.",
                EXO_NICO_STREAM, ".*");
        return;
    }

    rv = mlm_client_set_producer (self->client_Stream, EXO_NICO_STREAM);
    if (rv == -1) {
        mlm_client_destroy (&self->client_Stream);
        zsys_error ("mlm_client_set_consumer (stream = '%s') failed.", EXO_NICO_STREAM_RETURN);
        return;
    } 
    
    
    zpoller_t *poller = zpoller_new (pipe, mlm_client_msgpipe (self->client_Main), NULL);
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
        } else 
        if (which == mlm_client_msgpipe (self->client_Main)) {
            //Send a ping message to server
            //by Stream
            zmsg_t *asset_message = zmsg_new ();
            zmsg_addstr (asset_message, "PING");
            assert (asset_message);
            rv = mlm_client_send (self->client_Stream, "ping", &asset_message);
            assert (rv == 0);
            zmsg_t *message = mlm_client_recv (self->client_Stream);
            assert (message);
            char* msgReceive = zmsg_popstr (message);
            assert (streq (msgReceive, "GNIP"));
            zstr_free(&msgReceive);
            zmsg_destroy (&message);
            
            //by mailbox
            message = zmsg_new ();
            zmsg_addstr (message, "PING");
            rv = mlm_client_sendto (self->client_Mailbox, "ping", "ping", NULL, 1000, &message);
            assert (rv == 0);
            message = mlm_client_recv (self->client_Mailbox);
            assert (message);
            char *ret = zmsg_popstr (message);
            assert (ret!=NULL);
            assert (streq (ret, "GNIP"));
            zstr_free (&ret);
            zmsg_destroy (&message);
            
        }
    }
    
    //delete client object
    exo_nico_client_destroy (&self);
    zpoller_destroy (&poller);
}



//  --------------------------------------------------------------------------
//  Self test of this class

void
exo_nico_client_test (bool verbose)
{
    printf (" * exo_nico_client: ");

    //  @selftest
    //  Simple create/destroy test

    exo_nico_client_t *self = exo_nico_client_new ();
    assert (self);
    assert(self->client_Stream);
    assert(self->client_Mailbox);
    assert(self->client_Main);
    exo_nico_client_destroy (&self);
    //  @end
    printf ("OK\n");
}

