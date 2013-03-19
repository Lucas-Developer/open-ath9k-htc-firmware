#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define MAX_READ_SIZE	80

uint32_t checksum = 0;

void write_file(FILE *out, unsigned char *buf, uint32_t size, unsigned char *endian, unsigned char nl)
{
	int i=0;
	unsigned char tmp_buf[4];

	for(i=0; i<size; i+=4)
	{

		if( nl==1 )
		{
			if(i%16 == 0) {
				fprintf(out, "\n");
			}
        	    tmp_buf[0] = buf[i];
        	    tmp_buf[1] = buf[i+1];
        	    tmp_buf[2] = buf[i+2];
        	    tmp_buf[3] = buf[i+3];

			fprintf(out, "0x%08X, ", *((uint32_t *)(&tmp_buf[0])));

        } else {
            
            if(i%16 == 0) {
                fprintf(out, "\n");
			}
			tmp_buf[0] = buf[i+3];
			tmp_buf[1] = buf[i+2];
			tmp_buf[2] = buf[i+1];
			tmp_buf[3] = buf[i+0];
			fprintf(out, "0x%08X, ", *((uint32_t *)(&tmp_buf[0])));
		}
        checksum = checksum ^ *((uint32_t *)(&tmp_buf[0]));
	}
}

void write_rom(FILE *out, FILE *in)
{
	int size;
	long file_size;
	unsigned char buffer[MAX_READ_SIZE];
	int multiple = 0;

	file_size = size = 0;

	while(1)
	{
		size = fread(buffer, sizeof(unsigned char), sizeof(buffer), in);
		file_size += size;

		//write_file(out, buffer, size, NULL, 0);
		if( size == 0 )
		{
			if (multiple)
				fprintf(out, "%08X\n", checksum);

			goto ERR_DONE;
		}
		else if (size<MAX_READ_SIZE)
		{
		    multiple = 0;
			write_file(out, buffer, size, NULL, 1);
			fprintf(out, "%08X\n", checksum);
			goto ERR_DONE;
		}
		else if (size==MAX_READ_SIZE)
		{
			multiple = 1;
			write_file(out, buffer, MAX_READ_SIZE, NULL, 1);
	    }
	    else
	        goto ERR_DONE;
	}

ERR_DONE:

	return;
}


void write_array(FILE *out, FILE *in, unsigned char hif)
{
	int size;
	long file_size;
	unsigned char buffer[MAX_READ_SIZE];
	int multiple = 0;

	file_size = size = 0;

//	fprintf(out, "#include \"80211core_sh.h\"\n");
	fprintf(out, "#include <stdint.h>\n");
	fprintf(out, "const uint32_t zcFwImage[] = {\n");
	while(1)
	{
		size = fread(buffer, sizeof(unsigned char), sizeof(buffer), in);
		file_size += size;
		if( size == 0 )
		{
			if (multiple)
			{
				fprintf(out, "0x%08X\n", checksum);
				file_size += 4;
			}

			fprintf(out, "};\n");
			fprintf(out, "\nconst uint32_t zcFwImageSize=%ld;\n", file_size);

			goto ERR_DONE;
		}
		else if (size<MAX_READ_SIZE)
		{
			multiple = 0;

			write_file(out, buffer, size, NULL, hif);
			fprintf(out, "0x%08X\n", checksum);

			if( (size%4)!=0 )
			    file_size += (4-(size%4));

			file_size += 4;
			fprintf(out, "};\n");
			fprintf(out, "\nconst uint32_t zcFwImageSize=%ld;\n", file_size);

			goto ERR_DONE;
		}
		else if (size==MAX_READ_SIZE)
		{
			multiple = 1;
			write_file(out, buffer, MAX_READ_SIZE, NULL, hif);
		}
		else
			goto ERR_DONE;
	}

ERR_DONE:
	return;
}


int main(int argc, char* argv[])
{
	FILE *in, *out;
	int retVal;
	int i=0;
	char input_file_name[80];
	char output_file_name[80];

	in = out = 0x0;

	if( argc < 3 )
	{
		printf("\"bin2hex [input_file] [output_file] - gen array data\"!\n\r");
		printf("\"bin2hex [input_file] [output_file] [rom]- gen rom code\"!\n\r");
		goto ERR_DONE;
	}
	strcpy(input_file_name, argv[1]);
	strcpy(output_file_name, argv[2]);

	printf("bin2h %s %s!\n\r", input_file_name, output_file_name);

	if((in = fopen(input_file_name,"rb")) == NULL)
		goto ERR_DONE;

	if((out = fopen(output_file_name,"wt")) == NULL)
		goto ERR_DONE;

	if( !strcmp(argv[3],"rom"))
        /* for loading into RAM directly, e.g ROM code or patch code */
        write_rom(out, in);	
    else { 
        if(!strcmp(argv[4],"usb")) 
            write_array(out, in, 1);	/* for generating firmware (usb) */
        else if (!strcmp(argv[4],"pci")) 
            write_array(out, in, 0);	/* for generating firmware (pci) */
        else
            write_array(out, in, 1);	/* Default case firmware (usb) */
    }

ERR_DONE:

	if(in) fclose(in);
	if(out) fclose(out);

	return 0;

}
