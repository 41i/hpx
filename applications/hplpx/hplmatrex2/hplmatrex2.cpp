#include <hpx/runtime/components/component_factory.hpp>

#include <hpx/util/portable_binary_iarchive.hpp>
#include <hpx/util/portable_binary_oarchive.hpp>

#include <boost/serialization/version.hpp>
#include <boost/serialization/export.hpp>

#include "server/hplmatrex2.hpp"

///////////////////////////////////////////////////////////////
HPX_REGISTER_COMPONENT_MODULE();

HPX_REGISTER_MINIMAL_COMPONENT_FACTORY(
	hpx::components::simple_component<hpx::components::server::HPLMatrex2>,
	HPLMatrex2);

//Register the actions
HPX_REGISTER_ACTION_EX(
	hpx::components::server::HPLMatrex2::construct_action,HPLconstruct_action);
HPX_REGISTER_ACTION_EX(
	hpx::components::server::HPLMatrex2::destruct_action,HPLdestruct_action);
HPX_REGISTER_ACTION_EX(
	hpx::components::server::HPLMatrex2::allocate_action,HPLallocate_action);
HPX_REGISTER_ACTION_EX(
	hpx::components::server::HPLMatrex2::assign_action,HPLassign_action);
HPX_REGISTER_ACTION_EX(
	hpx::components::server::HPLMatrex2::set_action,HPLset_action);
HPX_REGISTER_ACTION_EX(
	hpx::components::server::HPLMatrex2::get_action,HPLget_action);
HPX_REGISTER_ACTION_EX(
	hpx::components::server::HPLMatrex2::solve_action,HPLsolve_action);
HPX_REGISTER_ACTION_EX(
	hpx::components::server::HPLMatrex2::gtop_action,HPLgtop_action);
HPX_REGISTER_ACTION_EX(
	hpx::components::server::HPLMatrex2::gleft_action,HPLgleft_action);
HPX_REGISTER_ACTION_EX(
	hpx::components::server::HPLMatrex2::gtrail_action,HPLgtrail_action);
HPX_REGISTER_ACTION_EX(
	hpx::components::server::HPLMatrex2::bsubst_action,HPLbsubst_action);
HPX_REGISTER_ACTION_EX(
	hpx::components::server::HPLMatrex2::check_action,HPLcheck_action);

HPX_DEFINE_GET_COMPONENT_TYPE(hpx::components::server::HPLMatrex2);
