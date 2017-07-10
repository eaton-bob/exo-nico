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
    rv = mlm_client_connect (self->client_Stream, endpoint, 1000, "stream");
    assert (rv == 0);

    rv = mlm_client_set_consumer (self->client_Stream, EXO_NICO_STREAM_RETURN, ".*");
    if (rv == -1) {
        mlm_client_destroy (&self->client_Stream);
        zsys_error (
                "mlm_client_set_consumer (stream = '%s', pattern = '%s') failed.",
                EXO_NICO_STREAM_RETURN, ".*");
        return;
    }

    rv = mlm_client_set_producer (self->client_Stream, EXO_NICO_STREAM);
    if (rv == -1) {
        mlm_client_destroy (&self->client_Stream);
        zsys_error ("mlm_client_set_consumer (stream = '%s') failed.", EXO_NICO_STREAM);
        return;
    } 
    
    
    zpoller_t *poller = zpoller_new (pipe, mlm_client_msgpipe (self->client_Main),
    mlm_client_msgpipe (self->client_Mailbox),mlm_client_msgpipe (self->client_Stream),NULL);
    zsock_signal (pipe, 0);
    zsys_debug ("actor client ready\n");
    
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
        
        if (which == mlm_client_msgpipe (self->client_Main)) {
            zmsg_t *messageMain = mlm_client_recv (self->client_Main);
            zmsg_destroy (&messageMain);

            //Send a ping message to server
            //by Stream
            printf("Ping Stream\n");
            zmsg_t *messagePing = zmsg_new ();
            assert (messagePing);
            zmsg_addstr (messagePing, "PING");
            rv = mlm_client_send (self->client_Stream, "ping", &messagePing);
            assert (rv == 0);
            
            //by mailbox
            printf("Ping mailBox \n");
            messagePing = zmsg_new ();
            assert (messagePing);
            zmsg_addstr (messagePing, "PING");
            rv = mlm_client_sendto (self->client_Mailbox, "exo-nico", "ping", NULL, 1000, &messagePing);
            assert (rv == 0);
      
        }
        
        if (which == mlm_client_msgpipe (self->client_Stream)) {
            zmsg_t *messageS = mlm_client_recv (self->client_Stream);
            assert (messageS);
            char* msgReceive = zmsg_popstr (messageS);
            printf("retour ping stream : %s\n",msgReceive);
            assert (streq (msgReceive, "GNIP"));
            zmsg_destroy (&messageS);
            zstr_free(&msgReceive);
        
        }
        if (which == mlm_client_msgpipe (self->client_Mailbox)) {
            zmsg_t* messageMB = mlm_client_recv (self->client_Mailbox);
            assert (messageMB);
            char *ret = zmsg_popstr (messageMB);
            assert (ret);
            printf("retour ping mailbox : %s\n",ret);
            assert (streq (ret, "GNIP"));
            zmsg_destroy (&messageMB);
            zstr_free (&ret);
        
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
    printf (" * exo_nico_client: \n");

    //  @selftest
    //  Simple create/destroy test

    exo_nico_client_t *self = exo_nico_client_new ();
    assert (self);
    assert(self->client_Stream);
    assert(self->client_Mailbox);
    assert(self->client_Main);
    exo_nico_client_destroy (&self);
   
     printf (" client test : \n");

    //  @selftest
    static const char* endpoint = "inproc://exo-nico-server-test";      

        //  Set up broker, fty example server actor and third party actor
    zactor_t *server = zactor_new (mlm_server, (void*)"Malamute");
    zstr_sendx (server, "BIND", endpoint, NULL);
    if (verbose)
        zstr_send (server, "VERBOSE");

    zactor_t *example_server = zactor_new (exo_nico_client, (void *) endpoint);

    printf("MailBox clients\n");
    //  Test MAILBOX deliver
    mlm_client_t * mailBoxClient = mlm_client_new();
    assert (mailBoxClient);

    int rv = mlm_client_connect (mailBoxClient, endpoint, 1000, "mainT");
    assert (rv == 0);
    
    mlm_client_t * mailBoxReturn = mlm_client_new();
    assert (mailBoxReturn);

    rv = mlm_client_connect (mailBoxReturn, endpoint, 1000, "exo-nico");
    assert (rv == 0);
    
    printf("Stream client\n");
     //get client object
    mlm_client_t * streamClient = mlm_client_new();
    assert (streamClient);

    rv = mlm_client_connect (streamClient, endpoint, 1000, "pingS");
    assert (rv == 0);
    rv = mlm_client_set_consumer (streamClient, EXO_NICO_STREAM, ".*");
    assert (rv == 0);
    rv = mlm_client_set_producer (streamClient, EXO_NICO_STREAM_RETURN);
    assert (rv == 0);
    
    //Send a message
    printf ("Envoi sur main\n");
    zmsg_t *messageMain = zmsg_new ();
    zmsg_addstr (messageMain, "DummyMsg");
    rv = mlm_client_sendto (mailBoxClient, "main", "ping", NULL, 1000, &messageMain);
    assert (rv == 0);
    
    printf ("Reception Stream\n");
    zmsg_t * messageS = mlm_client_recv (streamClient);
    assert (messageS);
    char * msgReceive = zmsg_popstr (messageS);
    printf("Message Stream : %s\n",msgReceive);
    assert (streq (msgReceive, "PING"));
    zmsg_destroy (&messageS);
    zstr_free(&msgReceive);
    zmsg_t *messageReturnS = zmsg_new ();
    zmsg_addstr (messageReturnS, "GNIP");
    assert (messageReturnS);
    rv = mlm_client_send (streamClient, "ping", &messageReturnS);
    assert (rv == 0);

    printf ("Reception MailBox\n");
    zmsg_t * messageMB = mlm_client_recv (mailBoxReturn);
    assert (messageMB);
    char *ret = zmsg_popstr (messageMB);

    printf("Message Mailbox : %s \n",ret);
    assert (streq (ret, "PING"));
    zmsg_destroy (&messageMB);
    zstr_free (&ret);
    zmsg_t *messageReturnMB = zmsg_new ();
    zmsg_addstr (messageReturnMB, "GNIP");
    rv = mlm_client_sendto (mailBoxReturn, "ping", "ping", NULL, 1000, &messageReturnMB);
    assert (rv == 0);
      
    mlm_client_destroy(&mailBoxReturn);
    mlm_client_destroy(&mailBoxClient);
    mlm_client_destroy(&streamClient);
    zactor_destroy (&example_server);
    zactor_destroy (&server);

    //  @end
    printf ("OK\n");

}

