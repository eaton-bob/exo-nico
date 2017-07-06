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
};

//getter Stream client
mlm_client_t * getClientStream(exo_nico_client_t *self)
{
    return self->client_Stream;
}

//getter MailBox client
mlm_client_t * getClientMailBox(exo_nico_client_t *self)
{
    return self->client_Mailbox;
}


//  --------------------------------------------------------------------------
//  Create a new exo_nico_client

exo_nico_client_t *
exo_nico_client_new (void)
{
    exo_nico_client_t *self = (exo_nico_client_t *) zmalloc (sizeof (exo_nico_client_t));
    assert (self);
    self->client_Stream =  mlm_client_new ();
    self->client_Mailbox =  mlm_client_new ();
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
        //  Free object itself
        free (self);
        *self_p = NULL;
    }
}

//  --------------------------------------------------------------------------
//  Self test of this class

void
exo_nico_client_test (bool verbose)
{
    printf (" * exo_nico_client: ");

    //  @selftest
    //  Simple create/destroy test

    // Note: If your selftest reads SCMed fixture data, please keep it in
    // src/selftest-ro; if your test creates filesystem objects, please
    // do so under src/selftest-rw. They are defined below along with a
    // usecase for the variables (assert) to make compilers happy.
    const char *SELFTEST_DIR_RO = "src/selftest-ro";
    const char *SELFTEST_DIR_RW = "src/selftest-rw";
    assert (SELFTEST_DIR_RO);
    assert (SELFTEST_DIR_RW);
    // Uncomment these to use C++ strings in C++ selftest code:
    //std::string str_SELFTEST_DIR_RO = std::string(SELFTEST_DIR_RO);
    //std::string str_SELFTEST_DIR_RW = std::string(SELFTEST_DIR_RW);
    //assert ( (str_SELFTEST_DIR_RO != "") );
    //assert ( (str_SELFTEST_DIR_RW != "") );
    // NOTE that for "char*" context you need (str_SELFTEST_DIR_RO + "/myfilename").c_str()

    exo_nico_client_t *self = exo_nico_client_new ();
    assert (self);
    assert(self->client_Stream);
    assert(self->client_Mailbox);
    exo_nico_client_destroy (&self);
    //  @end
    printf ("OK\n");
}
