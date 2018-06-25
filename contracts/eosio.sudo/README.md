eosio.sudo
--------

Actions:
The naming convention is codeaccount::actionname followed by a list of parameters.

Create a proposal
## eosio.sudo::propose    proposer proposal_name trx
   - **proposer** account proposing a transaction
   - **proposal_name** name of the proposal (should be unique for proposer)
   - **trx** proposed transaction

   Storage changes are billed to 'proposer'

Approve a proposal
## eosio.sudo::approve    proposer proposal_name approver
   - **proposer** account proposing a transaction
   - **proposal_name** name of the proposal
   - **approver** account name of an active block producer approving the transaction

   Storage changes are billed to 'proposer'

Revoke an approval of transaction
## eosio.sudo::unapprove    proposer proposal_name unapprover
   - **proposer** account proposing a transaction
   - **proposal_name** name of the proposal
   - **unapprover** account revoking prior approval from the transaction

   Storage changes are billed to 'proposer'

Cancel a proposal
## eosio.sudo::cancel    proposer proposal_name canceler
   - **proposer** account proposing a transaction
   - **proposal_name** name of the proposal
   - **canceler** account canceling the transaction (only proposer can cancel an unexpired transaction)

Execute a proposal
## eosio.sudo::exec    proposer proposal_name executer
   - **proposer** account proposing a transaction
   - **proposal_name** name of the proposal
   - **executer** account executing the transaction


Cleos usage example.

TODO: Fill in cleos usage example.
