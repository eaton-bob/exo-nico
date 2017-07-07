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

#ifndef EXO_NICO_CLIENT_H_INCLUDED
#define EXO_NICO_CLIENT_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

//  @interface
//  Create a new exo_nico_client
EXO_NICO_EXPORT exo_nico_client_t *
    exo_nico_client_new (void);

//  Destroy the exo_nico_client
EXO_NICO_EXPORT void
    exo_nico_client_destroy (exo_nico_client_t **self_p);

//  Self test of this class
EXO_NICO_EXPORT void
    exo_nico_client_test (bool verbose);

// actor action
EXO_NICO_EXPORT void
    exo_nico_client(zsock_t *pipe, void* args);
        
        

//  @end

#ifdef __cplusplus
}
#endif

#endif
