/*****************************************************************************

 @(#) $RCSfile$ $Name$($Revision$) $Date$

 -----------------------------------------------------------------------------

 Copyright (c) 2001-2007  OpenSS7 Corporation <http://www.openss7.com/>
 Copyright (c) 1997-2000  Brian F. G. Bidulock <bidulock@openss7.org>

 All Rights Reserved.

 This program is free software: you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation, version 3 of the license.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 details.

 You should have received a copy of the GNU General Public License along with
 this program.  If not, see <http://www.gnu.org/licenses/>, or write to the
 Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

 -----------------------------------------------------------------------------

 U.S. GOVERNMENT RESTRICTED RIGHTS.  If you are licensing this Software on
 behalf of the U.S. Government ("Government"), the following provisions apply
 to you.  If the Software is supplied by the Department of Defense ("DoD"), it
 is classified as "Commercial Computer Software" under paragraph 252.227-7014
 of the DoD Supplement to the Federal Acquisition Regulations ("DFARS") (or any
 successor regulations) and the Government is acquiring only the license rights
 granted herein (the license rights customarily provided to non-Government
 users).  If the Software is supplied to any unit or agency of the Government
 other than DoD, it is classified as "Restricted Computer Software" and the
 Government's rights in the Software are defined in paragraph 52.227-19 of the
 Federal Acquisition Regulations ("FAR") (or any successor regulations) or, in
 the cases of NASA, in paragraph 18.52.227-86 of the NASA Supplement to the FAR
 (or any successor regulations).

 -----------------------------------------------------------------------------

 Commercial licensing and support of this software is available from OpenSS7
 Corporation at a fee.  See http://www.openss7.com/

 -----------------------------------------------------------------------------

 Last Modified $Date$ by $Author$

 -----------------------------------------------------------------------------

 $Log$
 *****************************************************************************/

#ident "@(#) $RCSfile$ $Name$($Revision$) $Date$"

static char const ident[] = "$RCSfile$ $Name$($Revision$) $Date$";

/* dsapresult.c - DSAP : Send operation results */

#ifndef	lint
static char *rcsid =
    "Header: /xtel/isode/isode/dsap/net/RCS/dsapresult.c,v 9.0 1992/06/16 12:14:05 isode Rel";
#endif

/* 
 * Header: /xtel/isode/isode/dsap/net/RCS/dsapresult.c,v 9.0 1992/06/16 12:14:05 isode Rel
 *
 *
 * Log: dsapresult.c,v
 * Revision 9.0  1992/06/16  12:14:05  isode
 * Release 8.0
 *
 */

/*
 *                                NOTICE
 *
 *    Acquisition, use, and distribution of this module and related
 *    materials are subject to the restrictions of a license agreement.
 *    Consult the Preface in the User's Manual for the full terms of
 *    this agreement.
 *
 */

/* LINTLIBRARY */

#include <stdio.h>
#include "logger.h"
#include "quipu/dsap.h"
#include "../x500as/DAS-types.h"
#include "../x500as/Quipu-types.h"

extern LLog *log_dsap;

int
DapResultRequest(sd, id, res, di)
	int sd;
	int id;
	struct DSResult *res;
	struct DSAPindication *di;
{
	int result;
	PE res_pe;
	struct RoSAPindication roi_s;
	struct RoSAPindication *roi = &(roi_s);
	struct RoSAPpreject *rop = &(roi->roi_preject);

	if (DapEncodeResult(&(res_pe), res) != OK) {
		LLOG(log_dsap, LLOG_EXCEPTIONS, ("DapResultRequest: Encoding failed"));
		return (dsapreject(di, DP_INVOKE, id, NULLCP, "Failed to encode operation result"));
	}

	watch_dog("RoResultRequest (DAP)");
	result = RoResultRequest(sd, id, res->result_type, res_pe, ROS_NOPRIO, roi);
	watch_dog_reset();

	if (result != OK) {
		if (ROS_FATAL(rop->rop_reason) || (rop->rop_reason == ROS_PARAMETER)) {
			LLOG(log_dsap, LLOG_EXCEPTIONS, ("DapResultRequest(): Fatal rejection"));
			return (dsaplose(di, DP_INVOKE, NULLCP, "RoResultRequest failed"));
		} else {
			LLOG(log_dsap, LLOG_EXCEPTIONS,
			     ("DapResultRequest(): Non-Fatal rejection"));
			return (dsapreject(di, DP_INVOKE, id, NULLCP, "RoResultRequest failed"));
		}
	}

	if (res_pe != NULLPE)
		pe_free(res_pe);
	return (OK);
}

int
DapEncodeResult(pep, res)
	PE *pep;
	struct DSResult *res;
{
	int success;

	switch (res->result_type) {
	case OP_READ:
		success = encode_DAS_ReadResult(pep, 1, 0, NULLCP, &(res->res_rd));
		break;
	case OP_COMPARE:
		success = encode_DAS_CompareResult(pep, 1, 0, NULLCP, &(res->res_cm));
		break;
	case OP_ABANDON:
		success = encode_DAS_AbandonResult(pep, 1, 0, NULLCP, NULLCP);
		break;
	case OP_LIST:
		success = encode_DAS_ListResult(pep, 1, 0, NULLCP, &(res->res_ls));
		break;
	case OP_SEARCH:
		merge_search_results(&res->res_sr, res->res_sr.srr_next);
		success = encode_DAS_SearchResult(pep, 1, 0, NULLCP, &(res->res_sr));
		break;
	case OP_ADDENTRY:
		success = encode_DAS_AddEntryResult(pep, 1, 0, NULLCP, NULLCP);
		break;
	case OP_REMOVEENTRY:
		success = encode_DAS_RemoveEntryResult(pep, 1, 0, NULLCP, NULLCP);
		break;
	case OP_MODIFYENTRY:
		success = encode_DAS_ModifyEntryResult(pep, 1, 0, NULLCP, NULLCP);
		break;
	case OP_MODIFYRDN:
		success = encode_DAS_ModifyRDNResult(pep, 1, 0, NULLCP, NULLCP);
		break;
	default:
		success = NOTOK;
		LLOG(log_dsap, LLOG_EXCEPTIONS,
		     ("DapEncodeResult(): unknown op type %d", res->result_type));
		break;
	}

	return (success);
}

int
DspResultRequest(sd, id, res, di)
	int sd;
	int id;
	struct ds_op_res *res;
	struct DSAPindication *di;
{
	int result;
	PE res_pe;
	struct RoSAPindication roi_s;
	struct RoSAPindication *roi = &(roi_s);
	struct RoSAPpreject *rop = &(roi->roi_preject);

	if (DspEncodeResult(&(res_pe), res) != OK) {
		LLOG(log_dsap, LLOG_EXCEPTIONS, ("DspResultRequest: Encoding failed"));
		return (dsapreject(di, DP_INVOKE, id, NULLCP, "Failed to encode operation result"));
	}

	watch_dog("RoResultRequest (DSP)");
	result = RoResultRequest(sd, id, res->dcr_dsres.result_type, res_pe, ROS_NOPRIO, roi);
	watch_dog_reset();

	if (result != OK) {
		if (ROS_FATAL(rop->rop_reason) || (rop->rop_reason == ROS_PARAMETER)) {
			LLOG(log_dsap, LLOG_EXCEPTIONS, ("DspResultRequest(): Fatal rejection"));
			return (dsaplose(di, DP_INVOKE, NULLCP, "RoResultRequest failed"));
		} else {
			LLOG(log_dsap, LLOG_EXCEPTIONS,
			     ("DspResultRequest(): Non-Fatal rejection"));
			return (dsapreject(di, DP_INVOKE, id, NULLCP, "RoResultRequest failed"));
		}
	}

	if (res_pe != NULLPE)
		pe_free(res_pe);
	return (OK);
}

int
DspEncodeResult(pep, res)
	PE *pep;
	struct ds_op_res *res;
{
	int success;

	switch (res->dcr_dsres.result_type) {
	case OP_READ:
		success = encode_DO_ChainedReadResult(pep, 1, 0, NULLCP, res);
		break;
	case OP_COMPARE:
		success = encode_DO_ChainedCompareResult(pep, 1, 0, NULLCP, res);
		break;
	case OP_ABANDON:
		success = encode_DAS_AbandonResult(pep, 1, 0, NULLCP, NULLCP);
		break;
	case OP_LIST:
		success = encode_DO_ChainedListResult(pep, 1, 0, NULLCP, res);
		break;
	case OP_SEARCH:
		merge_search_results(&res->dcr_dsres.res_sr, res->dcr_dsres.res_sr.srr_next);
		success = encode_DO_ChainedSearchResult(pep, 1, 0, NULLCP, res);
		break;
	case OP_ADDENTRY:
		success = encode_DO_ChainedAddEntryResult(pep, 1, 0, NULLCP, res);
		break;
	case OP_REMOVEENTRY:
		success = encode_DO_ChainedRemoveEntryResult(pep, 1, 0, NULLCP, res);
		break;
	case OP_MODIFYENTRY:
		success = encode_DO_ChainedModifyEntryResult(pep, 1, 0, NULLCP, res);
		break;
	case OP_MODIFYRDN:
		success = encode_DO_ChainedModifyRDNResult(pep, 1, 0, NULLCP, res);
		break;
	default:
		success = NOTOK;
		LLOG(log_dsap, LLOG_EXCEPTIONS,
		     ("DspEncodeResult(): unknown op type %d", res->dcr_dsres.result_type));
		break;
	}

	return (success);
}

int
QspResultRequest(sd, id, res, di)
	int sd;
	int id;
	struct ds_op_res *res;
	struct DSAPindication *di;
{
	int result;
	PE res_pe;
	struct RoSAPindication roi_s;
	struct RoSAPindication *roi = &(roi_s);
	struct RoSAPpreject *rop = &(roi->roi_preject);

	if (QspEncodeResult(&(res_pe), res) != OK) {
		LLOG(log_dsap, LLOG_EXCEPTIONS, ("QspResultRequest: Encoding failed"));
		return (dsapreject(di, DP_INVOKE, id, NULLCP, "Failed to encode operation result"));
	}

	watch_dog("RoResultRequest (QSP)");
	result = RoResultRequest(sd, id, res->dcr_dsres.result_type, res_pe, ROS_NOPRIO, roi);
	watch_dog_reset();

	if (result != OK) {
		if (ROS_FATAL(rop->rop_reason) || (rop->rop_reason == ROS_PARAMETER)) {
			LLOG(log_dsap, LLOG_EXCEPTIONS, ("QspResultRequest(): Fatal rejection"));
			return (dsaplose(di, DP_INVOKE, NULLCP, "RoResultRequest failed"));
		} else {
			LLOG(log_dsap, LLOG_EXCEPTIONS,
			     ("QspResultRequest(): Non-Fatal rejection"));
			return (dsapreject(di, DP_INVOKE, id, NULLCP, "RoResultRequest failed"));
		}
	}

	if (res_pe != NULLPE)
		pe_free(res_pe);
	return (OK);
}

int
QspEncodeResult(pep, res)
	PE *pep;
	struct ds_op_res *res;
{
	int success;

	switch (res->dcr_dsres.result_type) {
	case OP_READ:
		success = encode_DO_ChainedReadResult(pep, 1, 0, NULLCP, res);
		break;
	case OP_COMPARE:
		success = encode_DO_ChainedCompareResult(pep, 1, 0, NULLCP, res);
		break;
	case OP_ABANDON:
		success = encode_DAS_AbandonResult(pep, 1, 0, NULLCP, NULLCP);
		break;
	case OP_LIST:
		success = encode_DO_ChainedListResult(pep, 1, 0, NULLCP, res);
		break;
	case OP_SEARCH:
		merge_search_results(&res->dcr_dsres.res_sr, res->dcr_dsres.res_sr.srr_next);
		success = encode_DO_ChainedSearchResult(pep, 1, 0, NULLCP, res);
		break;
	case OP_ADDENTRY:
		success = encode_DO_ChainedAddEntryResult(pep, 1, 0, NULLCP, res);
		break;
	case OP_REMOVEENTRY:
		success = encode_DO_ChainedRemoveEntryResult(pep, 1, 0, NULLCP, res);
		break;
	case OP_MODIFYENTRY:
		success = encode_DO_ChainedModifyEntryResult(pep, 1, 0, NULLCP, res);
		break;
	case OP_MODIFYRDN:
		success = encode_DO_ChainedModifyRDNResult(pep, 1, 0, NULLCP, res);
		break;
	case OP_GETEDB:
		success =
		    encode_Quipu_GetEntryDataBlockResult(pep, 1, 0, NULLCP,
							 &(res->dcr_dsres.res_ge));
		break;
	default:
		success = NOTOK;
		LLOG(log_dsap, LLOG_EXCEPTIONS,
		     ("QspEncodeResult(): unknown op type %d", res->dcr_dsres.result_type));
		break;
	}

	return (success);
}

int
IspResultRequest(sd, id, res, di)
	int sd;
	int id;
	struct ds_op_res *res;
	struct DSAPindication *di;
{
	return QspResultRequest(sd, id, res, di);
}

int
IspEncodeResult(pep, res)
	PE *pep;
	struct ds_op_res *res;
{
	return QspEncodeResult(pep, res);
}