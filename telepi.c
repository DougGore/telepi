/******************************************************************************
 * TelePi - remote streaming for your Raspberry Pi
 ******************************************************************************
 * Initial preview release
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "bcm_host.h"
#include "ilclient.h"

#include "telepi.h"

#define NUMFRAMES 300000
#define WIDTH     640
#define PITCH     ((WIDTH+31)&~31)
#define HEIGHT    480
#define HEIGHT16  ((HEIGHT+15)&~15)
#define SIZE      ((WIDTH * HEIGHT16 * 3)/2)

DISPMANX_DISPLAY_HANDLE_T   display;
DISPMANX_RESOURCE_HANDLE_T  resource;
DISPMANX_MODEINFO_T         info;
VC_RECT_T                   rect;


uint32_t        screen = 0;

uint32_t vc_image_ptr;

COMPONENT_T *video_encode = NULL;

OMX_BUFFERHEADERTYPE *buf;
OMX_BUFFERHEADERTYPE *out;

/*
* simulate grabbing a picture from some device
*/
int take_snapshot(void *buffer, OMX_U32 * filledLen)
{
   VC_IMAGE_TRANSFORM_T transform = 0;
   int ret;

   ret = vc_dispmanx_snapshot(display, resource, transform);
   assert(ret == 0);
   
   ret = vc_dispmanx_resource_read_data(resource, &rect, buffer, info.width * 3);
   assert(ret == 0);

   *filledLen = info.width * info.height * 3;

   return 1;
}


static int
video_encode_test(char *outputfilename)
{
   int status = 0;
   int framenumber = 0;
   FILE *outf;

   int ret;

   OMX_ERRORTYPE r;

   VC_IMAGE_TYPE_T type = VC_IMAGE_BGR888;

   fprintf(stderr, "Open display[%i]...\n", screen );
   display = vc_dispmanx_display_open( screen );

   ret = vc_dispmanx_display_get_info(display, &info);
   assert(ret == 0);
   fprintf(stderr, "Display is %d x %d\n", info.width, info.height );

   resource = vc_dispmanx_resource_create( type,
                                           info.width,
                                           info.height,
                                           &vc_image_ptr );

   fprintf(stderr, "VC image ptr: 0x%X\n", vc_image_ptr);

   ret = vc_dispmanx_rect_set(&rect, 0, 0, info.width, info.height);
   assert(ret == 0);

   encode_init(&video_encode);

   encode_config_format(video_encode, info.width, info.height, 30);
   encode_config_encoding(video_encode, OMX_VIDEO_CodingAVC);
   encode_config_bitrate(video_encode, 16000000 /* 8388608 */);

   //encode_config_low_latency(video_encode, true);

   encode_config_activate(video_encode);

   if (outputfilename[0] == '-')
   {
      outf = stdout;
   }
   else
   {
      outf = fopen(outputfilename, "wb");
   }
   
   if (outf == NULL)
   {
      fprintf(stderr, "Failed to open '%s' for writing video\n", outputfilename);
      exit(1);
   }

   fprintf(stderr, "looping for buffers...\n");
   do
   {
      buf = ilclient_get_input_buffer(video_encode, 200, 1);
      
      if (buf != NULL)
      {
      	 /* fill it */
         take_snapshot(buf->pBuffer, &buf->nFilledLen);
         framenumber++;

         if (OMX_EmptyThisBuffer(ILC_GET_HANDLE(video_encode), buf) != OMX_ErrorNone)
         {
            fprintf(stderr, "Error emptying buffer!\n");
         }

      	out = ilclient_get_output_buffer(video_encode, 201, 1);

      	r = OMX_FillThisBuffer(ILC_GET_HANDLE(video_encode), out);
      	if (r != OMX_ErrorNone)
         {
      	   fprintf(stderr, "Error filling buffer: %x\n", r);
      	}

      	if (out != NULL)
         {
            /*
      	   if (out->nFlags & OMX_BUFFERFLAG_CODECCONFIG)
            {
               int i;
               for (i = 0; i < out->nFilledLen; i++)
                  printf("%x ", out->pBuffer[i]);
               printf("\n");
            }
            */

            r = fwrite(out->pBuffer, 1, out->nFilledLen, outf);
            if (r != out->nFilledLen)
            {
               fprintf(stderr, "fwrite: Error emptying buffer: %d!\n", r);
            }
            else
            {
               //fprintf(stderr, "Writing frame %d/%d\n", framenumber, NUMFRAMES);
            }

            fflush(outf);

            out->nFilledLen = 0;
         }
         else
         {
            fprintf(stderr, "Not getting it :(\n");
         }

      }
      else
      {
         fprintf(stderr, "Doh, no buffers for me!\n");
      }

   } while (framenumber < NUMFRAMES);

   fclose(outf);

   fprintf(stderr, "Teardown.\n");

   encode_deinit(video_encode);

   return status;
}

int
main(int argc, char **argv)
{
   fprintf(stderr, "TelePi - Raspberry Pi remote viewer\n\n");

   if (argc < 2) {
      fprintf(stderr, "Usage:\n");
      fprintf(stderr, "Record to file: %s <filename>\n", argv[0]);
      fprintf(stderr, "Output to stdout: %s <filename>\n", argv[0]);

      fprintf(stderr, "\nStream to a remote computer:\n");
      fprintf(stderr, "%s - | netcat <remote_ip> 5001\n\n", argv[0]);
      exit(1);
   }

   bcm_host_init();

   return video_encode_test(argv[1]);
}
