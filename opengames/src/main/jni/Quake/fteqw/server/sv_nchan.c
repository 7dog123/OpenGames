/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// sv_nchan.c, user reliable data stream writes

#include "quakedef.h"
#ifndef CLIENTONLY

// check to see if client block will fit, if not, rotate buffers
void ClientReliableCheckBlock(client_t *cl, int maxsize)
{
	if (cl->num_backbuf ||
		cl->netchan.message.cursize > 
		cl->netchan.message.maxsize - maxsize - 1)
	{
		// we would probably overflow the buffer, save it for next
		if (!cl->num_backbuf)
		{
			memset(&cl->backbuf, 0, sizeof(cl->backbuf));
			cl->backbuf.allowoverflow = true;
			cl->backbuf.data = cl->backbuf_data[0];
			cl->backbuf.maxsize = sizeof(cl->backbuf_data[0]);
			cl->backbuf_size[0] = 0;
			cl->num_backbuf++;
		}

		if (cl->backbuf.cursize > cl->backbuf.maxsize - maxsize - 1)
		{
			if (cl->num_backbuf == MAX_BACK_BUFFERS)
			{
				cl->backbuf.cursize = 0; // don't overflow without allowoverflow set
				cl->netchan.message.overflowed = true; // this will drop the client
				if (!cl->drop)
					Con_Printf ("WARNING: MAX_BACK_BUFFERS for %s\n", cl->name);
				cl->drop = true;
				return;
			}
			memset(&cl->backbuf, 0, sizeof(cl->backbuf));
			cl->backbuf.allowoverflow = true;
			cl->backbuf.data = cl->backbuf_data[cl->num_backbuf];
			cl->backbuf.maxsize = sizeof(cl->backbuf_data[cl->num_backbuf]);
			cl->backbuf_size[cl->num_backbuf] = 0;
			cl->num_backbuf++;
		}
	}
}

// begin a client block, estimated maximum size
void ClientReliableWrite_Begin(client_t *cl, int c, int maxsize)
{
	if (cl->controller)
		Con_Printf("Writing %i to slave client's message buffer\n", c);
	ClientReliableCheckBlock(cl, maxsize);
	ClientReliableWrite_Byte(cl, c);
}

void ClientReliable_FinishWrite(client_t *cl)
{
	if (cl->num_backbuf)
	{
		cl->backbuf_size[cl->num_backbuf - 1] = cl->backbuf.cursize;

		if (cl->backbuf.overflowed)
		{
			if (!cl->netchan.message.overflowed)
				Con_TPrintf (STL_MESSAGEOVERFLOW,cl->num_backbuf,cl->name);
			cl->netchan.message.overflowed = true; // this will drop the client
		}
	}
}

void ClientReliableWrite_Angle(client_t *cl, float f)
{
	if (cl->num_backbuf)
	{
		MSG_WriteAngle(&cl->backbuf, f);
		ClientReliable_FinishWrite(cl);
	}
	else
		MSG_WriteAngle(&cl->netchan.message, f);
}

void ClientReliableWrite_Angle16(client_t *cl, float f)
{
	if (cl->num_backbuf)
	{
		MSG_WriteAngle16(&cl->backbuf, f);
		ClientReliable_FinishWrite(cl);
	}
	else
		MSG_WriteAngle16(&cl->netchan.message, f);
}

void ClientReliableWrite_Byte(client_t *cl, int c)
{
	if (cl->num_backbuf)
	{
		MSG_WriteByte(&cl->backbuf, c);
		ClientReliable_FinishWrite(cl);
	}
	else
		MSG_WriteByte(&cl->netchan.message, c);
}

void ClientReliableWrite_Char(client_t *cl, int c)
{
	if (cl->num_backbuf)
	{
		MSG_WriteChar(&cl->backbuf, c);
		ClientReliable_FinishWrite(cl);
	}
	else
		MSG_WriteChar(&cl->netchan.message, c);
}

void ClientReliableWrite_Float(client_t *cl, float f)
{
	if (cl->num_backbuf)
	{
		MSG_WriteFloat(&cl->backbuf, f);
		ClientReliable_FinishWrite(cl);
	}
	else
		MSG_WriteFloat(&cl->netchan.message, f);
}

void ClientReliableWrite_Coord(client_t *cl, float f)
{
	if (cl->num_backbuf)
	{
		MSG_WriteCoord(&cl->backbuf, f);
		ClientReliable_FinishWrite(cl);
	}
	else
		MSG_WriteCoord(&cl->netchan.message, f);
}

void ClientReliableWrite_Long(client_t *cl, int c)
{
	if (cl->num_backbuf)
	{
		MSG_WriteLong(&cl->backbuf, c);
		ClientReliable_FinishWrite(cl);
	}
	else
		MSG_WriteLong(&cl->netchan.message, c);
}

void ClientReliableWrite_Short(client_t *cl, int c)
{
	if (cl->num_backbuf)
	{
		MSG_WriteShort(&cl->backbuf, c);
		ClientReliable_FinishWrite(cl);
	}
	else
		MSG_WriteShort(&cl->netchan.message, c);
}
void ClientReliableWrite_Entity(client_t *cl, int c)
{
	if (cl->num_backbuf)
	{
		MSG_WriteEntity(&cl->backbuf, c);
		ClientReliable_FinishWrite(cl);
	}
	else
		MSG_WriteEntity(&cl->netchan.message, c);
}

void ClientReliableWrite_String(client_t *cl, char *s)
{
	if (cl->num_backbuf)
	{
		MSG_WriteString(&cl->backbuf, s);
		ClientReliable_FinishWrite(cl);
	}
	else
		MSG_WriteString(&cl->netchan.message, s);
}

void ClientReliableWrite_SZ(client_t *cl, void *data, int len)
{
	if (cl->num_backbuf)
	{
		SZ_Write(&cl->backbuf, data, len);
		ClientReliable_FinishWrite(cl);
	}
	else
		SZ_Write(&cl->netchan.message, data, len);
}



#ifdef PEXT_ZLIBDL

#ifdef WIN32
#define ZEXPORT VARGS
#include "../zip/zlib.h"
//# pragma comment (lib, "zip/zlib.lib") 
#else
#include <zlib.h>
#endif

void ClientReliableWrite_ZLib(client_t *cl, void *data, int len)
{
	int i;
	char out[MAX_QWMSGLEN*2];

	short *written = (short *)((char *)cl->netchan.message.data + cl->netchan.message.cursize);

	z_stream strm = {
		data,
		len,
		0,

		NULL,//out,
		sizeof(out),
		0,

		NULL,
		NULL,

		NULL,
		NULL,
		NULL,

		Z_BINARY,
		0,
		0
	};
	i=0;
	strm.next_out = out;

	deflateInit(&strm, Z_BEST_COMPRESSION);
	while(deflate(&strm, Z_FINISH) == Z_OK)
	{
		Sys_Error("Couldn't compile well\n");
//		ClientReliableWrite_SZ(cl, out, sizeof(out) - strm.avail_out);	//compress in chunks of 8192. Saves having to allocate a huge-mega-big buffer
//		i+=sizeof(out) - strm.avail_out;
//		strm.next_out = out;
//		strm.avail_out = sizeof(out);
	}
	if (strm.total_out > len)
	{
		ClientReliableWrite_Short(cl, 0);
		ClientReliableWrite_SZ(cl, data, len);
	}
	else
	{
		ClientReliableWrite_Short(cl, strm.total_out);
		ClientReliableWrite_SZ(cl, out, sizeof(out) - strm.avail_out);
	}
	i+=sizeof(out) - strm.avail_out;
	deflateEnd(&strm);		

//	return i;
}
#endif

#endif
