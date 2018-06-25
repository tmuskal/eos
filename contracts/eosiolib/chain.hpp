/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosiolib/chain.h>

namespace eosio {

   vector<account_name> get_active_producers() {
      size_t size = ::get_active_producers( (account_name*)0, 0 );
      eosio_assert( size % sizeof(account_name) == 0, "unexpected size for list of active producers" );

      vector<account_name> active_producers;
      active_producers.resize( (size / sizeof(account_name)) + 1 );
      // Adding the extra element to active_producers so that in the next call to get_active_producers
      // it is clear if it actually returned all active producers or if it was cut off because of the buffer size.

      size_t size2 = ::get_active_producers( (account_name*)active_producers.data(), active_producers.size() );
      eosio_assert( size == size2, "unable to retrieve list of active producers" );
      active_producers.pop_back(); // Get rid of the extra element at the end that was added earlier.

      return active_producers;
   }

}
