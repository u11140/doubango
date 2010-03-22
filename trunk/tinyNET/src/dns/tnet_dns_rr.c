/*
* Copyright (C) 2009 Mamadou Diop.
*
* Contact: Mamadou Diop <diopmamadou@yahoo.fr>
*	
* This file is part of Open Source Doubango Framework.
*
* DOUBANGO is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*	
* DOUBANGO is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*	
* You should have received a copy of the GNU General Public License
* along with DOUBANGO.
*
*/
/**@file tnet_dns_rr.c
 * @brief DNS Resource Record (RFC 1034 and 1035).
 *
 * @author Mamadou Diop <diopmamadou(at)yahoo.fr>
 *
 * @date Created: Sat Nov 8 16:54:58 2009 mdiop
 */
#include "tnet_dns_rr.h"

#include "tnet_dns_a.h"
#include "tnet_dns_aaaa.h"
#include "tnet_dns_cname.h"
#include "tnet_dns_mx.h"
#include "tnet_dns_naptr.h"
#include "tnet_dns_ns.h"
#include "tnet_dns_opt.h"
#include "tnet_dns_ptr.h"
#include "tnet_dns_soa.h"
#include "tnet_dns_srv.h"
#include "tnet_dns_txt.h"

#include "../tnet_types.h"

#include "tsk_memory.h"
#include "tsk_debug.h"
#include "tsk_string.h"

#include <string.h> /* strtok, strlen ... */

/**@ingroup tnet_dns_group
* Initializes any DNS RR (either NAPTR or SRV ...).
* @param rr The DNS RR to initialize.
* @param qtype The type of the RR.
* @param qclass The class of the RR.
* @retval Zero if succeed and non-zero error code otherwise.
*/
int tnet_dns_rr_init(tnet_dns_rr_t *rr, tnet_dns_qtype_t qtype, tnet_dns_qclass_t qclass)
{
	if(rr)
	{
		if(!rr->initialized)
		{
			rr->qtype = qtype;
			rr->qclass = qclass;
			
			rr->initialized = tsk_true;
			return 0;
		}
		return -2;
	}
	return -1;
}

/**@ingroup tnet_dns_group
* Deinitializes any DNS RR (either NAPTR or SRV ...).
* @param rr The DNS RR to deinitialize.
* @retval Zero if succeed and non-zero error code otherwise.
*/
int tnet_dns_rr_deinit(tnet_dns_rr_t *rr)
{
	if(rr)
	{
		if(rr->initialized)
		{
			TSK_FREE(rr->name);
			TSK_FREE(rr->rpdata);
			
			rr->initialized = tsk_false;
			return 0;
		}
		return -2;
	}
	return -1;
}

/**@ingroup tnet_dns_group
* Deserialize <character-string>.
*/
int tnet_dns_rr_charstring_deserialize(const void* data, size_t size, char** charstring, size_t *offset)
{
	/* RFC 1035 - 3.3. Standard RRs
		<character-string> is a single length octet followed by that number of characters. 
		<character-string> is treated as binary information, and can be up to 256 characters in
		length (including the length octet).
	*/
	uint8_t* dataPtr = (((uint8_t*)data)+ *offset);
	uint8_t length = *dataPtr;
	
	if(length < size)
	{
		*charstring = tsk_strndup((const char*)(dataPtr + 1), length);
		*offset += (1 + length);
		
		return 0;
	}
	else return -1;
}

/**@ingroup tnet_dns_group
* Deserializes a QName.
*/
int tnet_dns_rr_qname_deserialize(const void* data, size_t size, char** name, size_t *offset)
{
	/* RFC 1035 - 4.1.4. Message compression

		The pointer takes the form of a two octet sequence:
		+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
		| 1  1|                OFFSET                   |
		+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
	*/
	uint8_t* dataPtr = (((uint8_t*)data) + *offset);
	uint8_t* dataEnd = (dataPtr + size);
	unsigned usingPtr = 0; /* Do not change. */

	while(*dataPtr)
	{
		usingPtr = ((*dataPtr & 0xC0) == 0xC0);

		if(usingPtr)
		{
			size_t ptr_offset = (*dataPtr & 0x3F);
			ptr_offset = ptr_offset << 8 | *(dataPtr+1);
			
			*offset += 2;
			return tnet_dns_rr_qname_deserialize(data, size, name, &ptr_offset);
		}
		else
		{
			uint8_t length;

			if(*name)
			{
				tsk_strcat(name, ".");
			}	

			length = *dataPtr;
			*offset+=1, dataPtr++;

			tsk_strncat(name, (const char*)dataPtr, length);
			*offset += length, dataPtr += length;
		}
	}

	*offset+=1;

	return 0;
}

//int tnet_dns_rr_qname_deserialize(const void* data, size_t size, char** name, size_t *offset)
//{
//	/* RFC 1035 - 4.1.4. Message compression
//
//		The pointer takes the form of a two octet sequence:
//		+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//		| 1  1|                OFFSET                   |
//		+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//	*/
//	uint8_t* dataPtr = (((uint8_t*)data) + *offset);
//	uint8_t* dataEnd = (dataPtr + size);
//	unsigned usingPtr = 0; /* Do not change. */
//	unsigned islast = 0;
//
//	while(!islast)
//	{
//		usingPtr = ((*dataPtr & 0xC0) == 0xC0);
//
//		if(usingPtr)
//		{
//			uint8_t *Ptr;
//			uint16_t ptr_offset = (*dataPtr & 0x3F);
//			ptr_offset = ptr_offset << 8 | *(dataPtr+1);
//			Ptr = ((uint8_t*)data) + ptr_offset;
//			
//			tnet_qname_label_parse(Ptr, (dataEnd - Ptr), name, &islast);
//			*offset += 2, dataPtr += 2;
//		}
//		else
//		{
//			size_t length = tnet_qname_label_parse(dataPtr, size, name, &islast);
//			*offset += length, dataPtr += length;
//		}
//	}
//
//	*offset += usingPtr ? 0 : 1;
//
//	return 0;
//}

/**@ingroup tnet_dns_group
* Serializes a QName.
*/
int tnet_dns_rr_qname_serialize(const char* qname, tsk_buffer_t* output)
{
	/*
		QNAME       a domain name represented as a sequence of labels, where
					each label consists of a length octet followed by that
					number of octets.  The domain name terminates with the
					zero length octet for the null label of the root.  Note
					that this field may be an odd number of octets; no
					padding is used.

					Example: "doubango.com" ==> 8doubango3comNULL
	*/
	static uint8_t null = 0;

	if(qname)
	{
		char* _qname = tsk_strdup(qname);
		char* label = strtok(_qname, ".");

		while(label)
		{
			uint8_t length = strlen(label);
			tsk_buffer_append(output, &length, 1);
			tsk_buffer_append(output, label, strlen(label));

			label = strtok (0, ".");
		}

		TSK_FREE(_qname);
	}

	/* terminates domain name */
	tsk_buffer_append(output, &null, 1);

	return 0;
}

/**@ingroup tnet_dns_group
* Deserializes a DNS RR.
*/
tnet_dns_rr_t* tnet_dns_rr_deserialize(const void* data, size_t size, size_t* offset)
{
	tnet_dns_rr_t *rr = 0;
	uint8_t* dataStart = (uint8_t*)data;
	uint8_t* dataPtr = (dataStart + *offset);
	uint8_t* dataEnd = (dataPtr+size);
	tnet_dns_qtype_t qtype;
	tnet_dns_qclass_t qclass;
	uint32_t ttl;
	uint16_t rdlength;
	char* qname = 0;

	/* Check validity */
	if(!dataPtr || !size)
	{
		goto bail;
	}

	/* == Parse QNAME
	*/
	tnet_dns_rr_qname_deserialize(dataStart, size, &qname, offset);
	dataPtr = (dataStart + *offset);
	/* == Parse QTYPE
	*/
	qtype = (tnet_dns_qtype_t)ntohs(*((uint16_t*)dataPtr));
	dataPtr += 2, *offset += 2;
	/* == Parse QCLASS
	*/
	qclass = (tnet_dns_qclass_t)ntohs(*((uint16_t*)dataPtr));
	dataPtr += 2, *offset += 2;
	/* == Parse TTL
	*/
	ttl = ntohl(*((uint32_t*)dataPtr));
	dataPtr += 4, *offset += 4;
	/* == Parse RDLENGTH
	*/
	rdlength = ntohs(*((uint16_t*)dataPtr));
	dataPtr += 2, *offset += 2;

	switch(qtype)
	{
		case qtype_a:
			{
				rr = TNET_DNS_A_CREATE(qname, qclass, ttl, rdlength, dataStart, *offset);
				break;
			}

		case qtype_aaaa:
			{
				rr = TNET_DNS_AAAA_CREATE(qname, qclass, ttl, rdlength, dataStart, *offset);
				break;
			}

		case qtype_cname:
			{
				rr = TNET_DNS_CNAME_CREATE(qname, qclass, ttl, rdlength, dataStart, *offset);
				break;
			}

		case qtype_mx:
			{
				rr = TNET_DNS_MX_CREATE(qname, qclass, ttl, rdlength, dataStart, *offset);
				break;
			}

		case qtype_naptr:
			{
				rr = TNET_DNS_NAPTR_CREATE(qname, qclass, ttl, rdlength, dataStart, *offset);
				break;
			}

		case qtype_ns:
			{
				rr = TNET_DNS_NS_CREATE(qname, qclass, ttl, rdlength, dataStart, *offset);
				break;
			}

		case qtype_opt:
			{
				unsigned payload_size = qclass;
				rr = TNET_DNS_OPT_CREATE(payload_size);
				break;
			}

		case qtype_ptr:
			{
				rr = TNET_DNS_PTR_CREATE(qname, qclass, ttl, rdlength, dataStart, *offset);
				break;
			}

		case qtype_soa:
			{
				rr = TNET_DNS_SOA_CREATE(qname, qclass, ttl, rdlength, dataStart, *offset);
				break;
			}

		case qtype_srv:
			{
				rr = TNET_DNS_SRV_CREATE(qname, qclass, ttl, rdlength, dataStart, *offset);
				break;
			}

		case qtype_txt:
			{
				rr = TNET_DNS_TXT_CREATE(qname, qclass, ttl, rdlength, dataStart, *offset);
				break;
			}

		default:
			{
				TSK_DEBUG_ERROR("NOT IMPLEMENTED");
				break;
			}
	}

bail:
	TSK_FREE(qname);
	
	*offset += rdlength;
	return rr;
}

/**@ingroup tnet_dns_group
* Serializes a DNS RR.
*/
int tnet_dns_rr_serialize(const tnet_dns_rr_t* rr, tsk_buffer_t *output)
{
	if(!rr || !output)
	{
		return -1;
	}

	/* NAME
	*/
	{
		tnet_dns_rr_qname_serialize(rr->name, output);
	}
	
	/* TYPE
	*/
	{
		uint16_t qtype = htons(rr->qtype);
		tsk_buffer_append(output, &(qtype), 2);
	}

	/* CLASS
	*/
	{
		uint16_t qclass = htons(rr->qclass);
		tsk_buffer_append(output, &(qclass), 2);
	}

	/* TTL
	*/
	{
		uint32_t ttl = htonl(rr->ttl);
		tsk_buffer_append(output, &(ttl), 4);
	}

	/* RDLENGTH
	*/
	{
		uint16_t length = htons(rr->rdlength);
		tsk_buffer_append(output, &(length), 2);
	}
	
	/* RDATA
	*/
	
	switch(rr->qtype)
	{
		case qtype_a:
			{
				TSK_DEBUG_ERROR("NOT IMPLEMENTED");
				break;
			}

		case qtype_aaaa:
			{
				TSK_DEBUG_ERROR("NOT IMPLEMENTED");
				break;
			}

		case qtype_cname:
			{
				TSK_DEBUG_ERROR("NOT IMPLEMENTED");
				break;
			}

		case qtype_mx:
			{
				TSK_DEBUG_ERROR("NOT IMPLEMENTED");
				break;
			}

		case qtype_naptr:
			{
				TSK_DEBUG_ERROR("NOT IMPLEMENTED");
				break;
			}

		case qtype_ns:
			{
				TSK_DEBUG_ERROR("NOT IMPLEMENTED");
				break;
			}

		case qtype_opt:
			{
				TSK_DEBUG_ERROR("NOT IMPLEMENTED");
				break;
			}

		case qtype_ptr:
			{
				TSK_DEBUG_ERROR("NOT IMPLEMENTED");
				break;
			}

		case qtype_soa:
			{
				TSK_DEBUG_ERROR("NOT IMPLEMENTED");
				break;
			}

		case qtype_srv:
			{
				TSK_DEBUG_ERROR("NOT IMPLEMENTED");
				break;
			}

		case qtype_txt:
			{
				TSK_DEBUG_ERROR("NOT IMPLEMENTED");
				break;
			}

		default:
			{
				TSK_DEBUG_ERROR("NOT IMPLEMENTED");
				break;
			}
	}

	return -1;
}


//=================================================================================================
//	[[DNS RR]] object definition
//
static void* tnet_dns_rr_create(void * self, va_list * app)
{
	tnet_dns_rr_t *rr = self;
	if(rr)
	{
		tnet_dns_rr_init(rr, qtype_any, qclass_any);
	}
	return self;
}

static void* tnet_dns_rr_destroy(void * self) 
{ 
	tnet_dns_rr_t *rr = self;
	if(rr)
	{
		tnet_dns_rr_deinit(rr);
	}
	return self;
}

static const tsk_object_def_t tnet_dns_rr_def_s =
{
	sizeof(tnet_dns_rr_t),
	tnet_dns_rr_create,
	tnet_dns_rr_destroy,
	0,
};
const void *tnet_dns_rr_def_t = &tnet_dns_rr_def_s;