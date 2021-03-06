#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <math.h>
#include <getopt.h>
#include <sys/types.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include <fftw3.h>
#include <stdint.h>
#include <sys/stat.h>
#include <ctype.h>

#define N 80

int make_guppi_header(){

    FILE *f, *o;

    char line[N];
    char status;
    char *header_param;
    int namelen, paramlen;
    int header_pad = 0;
    int param_pad = 0;

    f = fopen("main_header.txt", "r");
    if(f==NULL){
        exit(EXIT_FAILURE);
    }

    o = fopen("guppi_header.txt", "w");
    if(o==NULL){
        exit(EXIT_FAILURE);
    }

    int num = 0;
    while(fgets(line, N, f)){
        
        // printf("Line  %s\n", line);
        
        if(line[0]==' ') continue;
        strtok_r(line,"=",&header_param);
        int j = 0;
        while(header_param[j] != '\0'){
            if(header_param[j]==' ' && header_param[j+1]==' '){
                header_param[j]='\0';
                break;
            }
            if(header_param[j]=='\n' && header_param[j+1]==' ' || header_param[j]=='\n' && header_param[j+1]=='\0'){
                header_param[j]='\0';
                break;
            }
            j += 1;
        }

        //Replace \0 of header_param with ''
        for(int i = 0; i < strlen(header_param); i++){
            if(header_param[i] == '\0') header_param[i] = ' ';
        }
        
        //Check if header_param is an integer, string or a float
        if(!isdigit(header_param[0])){
            if(strstr(line, "OBSBW") != NULL || strstr(line, "CHAN_BW") != NULL) status = 'f';
            else status = 's';
	    }
        else{
            if(strchr(header_param,'.') && !strchr(header_param,':')) status = 'f';
            else if(strchr(header_param,'/')) status = 's';
            else if(strchr(header_param,'S')) status = 's';
            else if(strchr(header_param,':')) status = 's';
            else status = 'i';
        }

        //Try to write it as it would be seen in a GUPPI Header
        char *header = (char*)malloc(N);
        namelen = strlen(line);
        paramlen = strlen(header_param);
        if(namelen <8) header_pad = 8 - namelen;

        int c = 1;
        int count = 0;
        while(count < N){

            //Pad header name 
            if(!header_pad){
                for(int l = 0; l < namelen; l++){
                    header[count] = line[count];
                    count += 1;
                } 
            } 
            else{
                for(count = 0; count < namelen; count++){
                    header[count] = line[count];
                }
                while(count <=7){
                    header[count] = ' ';
                    count += 1;
                }
            }

            //Add = and a space after it
            header[count] = '=';
            count += 1;
            header[count] = ' ';
            count += 1;

            // printf("Val of Count before writing the actual param = %d\n", count);

            //Pad according to type of header param
            if(status == 's'){
                header[count] = '\'';
                count += 1;
                int i = 0;
                while(i < paramlen){
                    header[count] = header_param[i];
                    count += 1;
                    i += 1;
                }
                if(paramlen < 8){
                    // printf("In here!\n");
                    int i = 0;
                    while(i < 8 - paramlen){
                        header[count] = ' ';
                        count += 1;
                        i += 1;
                    }
                    header[count] = '\'';
                    count += 1;
                }
                else {
                    header[count] = '\'';
                    count += 1;
                }

                while(count < N){
                    header[count] = ' ';
                    count += 1;
                }
                // printf("%s\n", header);
                //Remove \x00 from header
                for(int i = 0 ; i < N; i++){
                    if(header[i] == '\0') header[i]=' ';
                }

            }
            else{
                // printf("N - count - paramlen = %d - %d - %d = %d\n", N, count, paramlen, N - count - paramlen);
                int num_spaces = 20;
                for(int i = 0; i< num_spaces; i++){
                    if(num_spaces - i <= paramlen){
                        for(int j = 0; j< paramlen; j++){
                            header[count] = header_param[j];
                            count += 1;
                        }
                        break;
                    }
                    header[count] = ' ';
                    // printf("%d ", count);
                    count += 1;
                }
                // printf("\n");
                // printf("Value of Count = %d\n", count);

                num_spaces = N - count; 

                for(int i = 0; i < num_spaces; i++){
                    header[count] = ' ';
                    count += 1;
                }

            }

        }



        // printf("%s(%d) %s(%d)(%c)\n", line, namelen, header_param, paramlen, status);
        printf("%s\n", header);
        fwrite(header, sizeof(char), N, o);
        num += N;
        free(header);
    }
    

    //Write END at the end
    char *header = (char*)malloc(N);
    header[0] = 'E';
    header[1] = 'N';
    header[2] = 'D';
    for(int i = 3; i < N; i++){
        header[i] = ' ';
    }
    printf("%s\n", header);
    fwrite(header, sizeof(char), N, o);
    num += 80;

    printf("Size of all header params together is %d, the amount of padding bytes to be added is %d\n", num, 512 - (int)(num%512));
    //Add padding at the end
    int padding = 512 - (int)(num%512);
    char* padding_byte = (char *)malloc(padding);
    for(int l = 0; l < padding; l++){
        padding_byte[l] = 'p';
    }

    fwrite(padding_byte, sizeof(char), padding, o);

    printf("*****Done writing HEADER*****\n");


    fclose(f);
    fclose(o);
    free(header);
    free(padding_byte);
    // free(header_param);


    return 1;
}

int update_header_param(char* param, char* param_type, long double param_value){

    //If Header parameter to be changed is a string then the value of the string is passed into the param_type variable
    //param_value in that case is ignored
    if(!strcmp(param_type, "Lf") || !strcmp(param_type, "f") || !strcmp(param_type, "i")){
        printf("%s value = %Lf\n", param, param_value);
    }
    else{
        printf("%s = %s\n", param, param_type);
    }
    
    if(!strcmp(param_type, "Lf")){
        // printf("In here!\n");
        char command[128];
        snprintf(command, sizeof(command), "sed -i 's/^%s=.*/%s=%.11Lf/' main_header.txt", param, param, (long double) param_value);
        printf("Updating %s with {%s}\n", param, command);
        int systemRet = system(command);
        if(systemRet == -1){
            printf("%s SED failed\n", param);
            exit(1);
        }
    }
    else if(!strcmp(param_type, "f")){
        char command[128];
        snprintf(command, sizeof(command), "sed -i 's/^%s=.*/%s=%.2f/' main_header.txt", param, param, (double) param_value);
        printf("Updating %s with {%s}\n", param, command);
        int systemRet = system(command);
        if(systemRet == -1){
            printf("%s SED failed\n", param);
            exit(1);
        }
    }
    else if(!strcmp(param_type, "i")){
        char command[128];
        snprintf(command, sizeof(command), "sed -i 's/^%s=.*/%s=%d/' main_header.txt", param, param, (int) param_value);
        printf("Updating %s with {%s}\n", param, command);
        int systemRet = system(command);
        if(systemRet == -1){
            printf("%s SED failed\n", param);
            exit(1);
        }
    }
    else{
        char command[128];
        snprintf(command, sizeof(command), "sed -i 's/^%s=.*/%s=%s/' main_header.txt", param, param, param_type);
        printf("Updating %s with {%s}\n", param, command);
        int systemRet = system(command);
        if(systemRet == -1){
            printf("%s SED failed\n", param);
            exit(1);
        }
    }
}