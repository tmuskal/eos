#include <eosio.sudo/eosio.sudo.hpp>
#include <eosiolib/permission.hpp>
#include <eosiolib/chain.hpp>

#include <algorithm>
#include <boost/function_output_iterator.hpp>

namespace eosio {

/*
propose function manually parses input data (instead of taking parsed arguments from dispatcher)
because parsing data in the dispatcher uses too much CPU in case if proposed transaction is big

If we use dispatcher the function signature should be:

void sudo::propose( account_name proposer,
                    name proposal_name,
                    transaction  trx )
*/

void sudo::propose() {
   constexpr size_t max_stack_buffer_size = 512;
   size_t size = action_data_size();
   char* buffer = (char*)( max_stack_buffer_size < size ? malloc(size) : alloca(size) );
   read_action_data( buffer, size );

   account_name proposer;
   name proposal_name;
   transaction_header trx_header;

   datastream<const char*> ds( buffer, size );
   ds >> proposer >> proposal_name;

   size_t trx_pos = ds.tellp();
   ds >> trx_header;

   require_auth( proposer );
   eosio_assert( trx_header.expiration >= eosio::time_point_sec(now()), "transaction expired" );

   proposals proptable( _self, proposer );
   eosio_assert( proptable.find( proposal_name ) == proptable.end(), "proposal with the same name exists" );

   proptable.emplace( proposer, [&]( auto& prop ) {
      prop.proposal_name       = proposal_name;
      prop.packed_transaction  = bytes( buffer+trx_pos, buffer+size );
   });

   approvals apptable(  _self, proposer );
   apptable.emplace( proposer, [&]( auto& a ) {
      a.proposal_name       = proposal_name;
   });
}

void sudo::approve( account_name proposer, name proposal_name, account_name approver ) {
   require_auth( approver );

   approvals apptable(  _self, proposer );
   auto& apps = apptable.get( proposal_name, "proposal not found" );

   auto active_producers = get_active_producers();
   auto itr = std::find( active_producers.begin(), active_producers.end(), approver );
   eosio_assert( itr != active_producers.end(), "approver is not an active producer" );

   bool inserted = true;
   apptable.modify( apps, proposer, [&]( auto& a ) {
      std::tie( std::ignore, inserted ) = a.provided_approvals.insert( approver );
   });
   eosio_assert( inserted, "identical approval previously granted" );
}

void sudo::unapprove( account_name proposer, name proposal_name, account_name unapprover ) {
   require_auth( unapprover );

   approvals apptable(  _self, proposer );
   auto& apps = apptable.get( proposal_name, "proposal not found" );
   auto itr = apps.provided_approvals.find( unapprover );
   eosio_assert( itr != apps.provided_approvals.end(), "no approval previously granted" );

   apptable.modify( apps, proposer, [&]( auto& a ) {
      a.provided_approvals.erase( itr );
   });
}

void sudo::cancel( account_name proposer, name proposal_name, account_name canceler ) {
   require_auth( canceler );

   proposals proptable( _self, proposer );
   auto& prop = proptable.get( proposal_name, "proposal not found" );

   approvals apptable(  _self, proposer );
   auto& apps = apptable.get( proposal_name, "proposal not found" );

   if( canceler != proposer ) {
      eosio_assert( unpack<transaction_header>( prop.packed_transaction ).expiration < eosio::time_point_sec(now()),
                    "cannot cancel until expiration" );
   }

   proptable.erase(prop);
   apptable.erase(apps);
}

void sudo::exec( account_name proposer, name proposal_name, account_name executer ) {
   require_auth( executer );

   proposals proptable( _self, proposer );
   auto& prop = proptable.get( proposal_name, "proposal not found" );

   approvals apptable(  _self, proposer );
   auto& apps = apptable.get( proposal_name, "proposal not found" );

   transaction_header trx_header;
   datastream<const char*> ds( prop.packed_transaction.data(), prop.packed_transaction.size() );
   ds >> trx_header;
   eosio_assert( trx_header.expiration >= eosio::time_point_sec(now()), "transaction expired" );

   auto active_producers = get_active_producers();
   std::sort( active_producers.begin(), active_producers.end() );

   vector<permission_level> provided_permissions;
   provided_permissions.reserve( std::min( active_producers.size(), apps.provided_approvals.size() ) );

   std::set_intersection( active_producers.begin(), active_producers.end(),
                          apps.provided_approvals.begin(), apps.provided_approvals.end(),
                          boost::make_function_output_iterator( [&]( const account_name& producer )
   {
      provided_permissions.emplace_back( producer, N(active) );
   } ) );

   bytes packed_provided_permissions = pack( provided_permissions );
   auto res = ::check_permission_authorization( N(eosio.prods),
                                                N(active),
                                                (const char*)0, 0,
                                                packed_provided_permissions.data(), packed_provided_permissions.size(),
                                                trx_header.delay_sec.value * 1'000'000
                                              );
   eosio_assert( res > 0, "approvals do not satisfy eosio.prods@active" );

   send_deferred( (uint128_t(proposer) << 64) | proposal_name, executer, prop.packed_transaction.data(), prop.packed_transaction.size() );

   proptable.erase(prop);
   apptable.erase(apps);
}

} /// namespace eosio

EOSIO_ABI( eosio::sudo, (propose)(approve)(unapprove)(cancel)(exec) )
