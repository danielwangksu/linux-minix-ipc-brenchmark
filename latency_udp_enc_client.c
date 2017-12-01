/**************************************************************
*   Linux UDP Socket for Latency Testing     Client           *
*   by Daniel Wang                                            *
***************************************************************/

#include <stdio.h> /* For printf */
#include <stdlib.h> /* For exit() */
#include <unistd.h>
#include <sys/types.h>
#include <signal.h> /* For kill() */
#include <string.h>
#include <fcntl.h>  /* For O_* constants */
#include <sys/stat.h>   /* For mode constants */
#include <arpa/inet.h>  /* For socket */

#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>

#include <sys/resource.h>

#include <time.h>
#include <sys/time.h>

#define OK  0
#define BILLION  1000000000L
#define MSGSIZE 8*1024

#define DEST_UDP_PORT 9090 /* receiving port */

unsigned char buffsend[MSGSIZE] = {0};
unsigned char buffrecv[MSGSIZE] = {0};

unsigned char ptext[MSGSIZE] = {0};
unsigned char ctext[MSGSIZE] = {0};
unsigned char dtext[MSGSIZE] = {0};

// fault handler
static void bail(const char *on_what) {
    perror(on_what);
}

/* Error Handler */
void handleErrors(void)
{
    ERR_print_errors_fp(stderr);
    abort();
}

/*=================================================================*
 *              AES-encryption                            *
 *=================================================================*/
int encrypt(unsigned char *plaintext, int plaintext_len, unsigned char *key,
  unsigned char *iv, unsigned char *ciphertext)
{
    EVP_CIPHER_CTX *ctx;
    int len;
    int ciphertext_len;

    /* Create and initialise the context */
    if(!(ctx = EVP_CIPHER_CTX_new())) 
        handleErrors();

    /* Initialise the encryption operation. IMPORTANT - ensure you use a key
     * and IV size appropriate for your cipher
     * In this example we are using 256 bit AES (i.e. a 256 bit key). The
     * IV size for *most* modes is the same as the block size. For AES this
     * is 128 bits */
    if(1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv))
        handleErrors();

    /* Provide the message to be encrypted, and obtain the encrypted output.
     * EVP_EncryptUpdate can be called multiple times if necessary
     */
    if(1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len))
        handleErrors();
    
    ciphertext_len = len;

    /* Finalise the encryption. Further ciphertext bytes may be written at
     * this stage.
     */
    if(1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len)) handleErrors();
    ciphertext_len += len;

    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);

    return ciphertext_len;
}


/*=================================================================*
 *              AES-decryption                            *
 *=================================================================*/
int decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *key,
  unsigned char *iv, unsigned char *plaintext)
{
    EVP_CIPHER_CTX *ctx;

    int len;

    int plaintext_len;

    /* Create and initialise the context */
    if(!(ctx = EVP_CIPHER_CTX_new())) 
        handleErrors();

    /* Initialise the decryption operation. IMPORTANT - ensure you use a key
     * and IV size appropriate for your cipher
     * In this example we are using 256 bit AES (i.e. a 256 bit key). The
     * IV size for *most* modes is the same as the block size. For AES this
     * is 128 bits */
    if(1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv))
        handleErrors();

    /* Provide the message to be decrypted, and obtain the plaintext output.
     * EVP_DecryptUpdate can be called multiple times if necessary
     */
    if(1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len))
        handleErrors();
  
    plaintext_len = len;

    /* Finalise the decryption. Further plaintext bytes may be written at
     * this stage.
     */
    if(1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len)) 
        handleErrors();
    
    plaintext_len += len;

    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);

    return plaintext_len;
}

/*=================================================================*
 *              diff                            *
 *=================================================================*/
unsigned long diff(struct timespec start, struct timespec end)
{
    struct timespec temp;
    if((end.tv_nsec - start.tv_nsec) < 0)
    {
        temp.tv_sec = end.tv_sec - start.tv_sec -1;
        temp.tv_nsec = (BILLION + end.tv_nsec) - start.tv_nsec;
    } else {
        temp.tv_sec = end.tv_sec - start.tv_sec;
        temp.tv_nsec = end.tv_nsec - start.tv_nsec;
    }
    return (unsigned long)temp.tv_sec * BILLION + (unsigned long)temp.tv_nsec;
}

/*=================================================================*
 *              decrypt_message                 *
 *=================================================================*/
void decrypt_message(int cipherlen)
{
    int decryptedtext_len;
    // 256 bits key
    unsigned char *key = (unsigned char *) "0123456789abcdefghigklmnopqrstuv";
    // 128 bit IV
    unsigned char *iv = (unsigned char *) "0123456789abcdefg";

    decryptedtext_len = decrypt(buffrecv, cipherlen, key, iv, dtext);
    dtext[decryptedtext_len] = '\0';
    //printf("%s\n", decryptedtext);
}

/*=================================================================*
 *              prepare_message                 *
 *=================================================================*/
void randomptext()
{
    int i = 0;

    for(i = 0; i < MSGSIZE; i++)
        ptext[i] = 'A' + (random() % 26);
}

/*=================================================================*
 *              prepare_message                 *
 *=================================================================*/
int prepare_message(int size)
{
    int ciphertext_len;

    // 256 bits key
    unsigned char *key = (unsigned char *) "0123456789abcdefghigklmnopqrstuv";
    // 128 bit IV
    unsigned char *iv = (unsigned char *) "0123456789abcdefg";


    ciphertext_len = encrypt(ptext, size, key, iv, ctext);

    memcpy(buffsend, ctext, ciphertext_len);

    return ciphertext_len;
}


/*=================================================================*
 *              main                            *
 *=================================================================*/
int main(int argc, char ** argv)
{
    int i, nloop, msgsize, status, prio;
    char *destip;
    int cipherlen;

    struct sockaddr_in dest_adr;
    socklen_t dest_adr_len;
    int socket_fd;

    struct timespec before = {0, 0};
    struct timespec after = {0, 0};

    unsigned long elapse = 0;

    if(argc != 4)
        bail("[Error] usage: latency_udpsocket_client {NUM_OF_LOOPS} {MESSAGE_SIZE} {SERVER_IP}");

    nloop = atoi(argv[1]);
    msgsize = atoi(argv[2]);
    destip = argv[3];
    //printf("[CLIENT]: DEST IP = %s\n", destip);

    prio = setpriority(PRIO_PROCESS, 0, -20);
    if(prio != 0)
        printf("[CLIENT]: need more privilege to change priority\n");

    memset(&dest_adr, 0, sizeof dest_adr);
    dest_adr.sin_family = AF_INET;
    dest_adr.sin_port = htons(DEST_UDP_PORT);
    dest_adr.sin_addr.s_addr = inet_addr(destip);
    dest_adr_len = sizeof dest_adr;

    if (dest_adr.sin_addr.s_addr == INADDR_NONE)
        bail("[Error]: bad address");

    socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd == -1)
        bail("[Error]: socket() failed");

    ERR_load_crypto_strings();
    OpenSSL_add_all_algorithms();
    OPENSSL_config(NULL);

    randomptext();

    sleep(1);

    for(i = 0; i < nloop; i++)
    {   
        //printf("[CLIENT]: sending message to %s\n", inet_ntoa(dest_adr.sin_addr));
        clock_gettime(CLOCK_MONOTONIC, &before);
        cipherlen = prepare_message(msgsize);
        status = sendto(socket_fd, buffsend, cipherlen, 0, (struct sockaddr *)&dest_adr, dest_adr_len);
        if (status < 0)
            bail("[Error]: sendto() failed");
        status = recvfrom(socket_fd, buffrecv, sizeof(buffrecv), 0, (struct sockaddr *)&dest_adr, &dest_adr_len);
        if(status < 0)
            bail("[Error]: recvfrom() failed");

        decrypt_message(status);
        clock_gettime(CLOCK_MONOTONIC, &after);

        elapse += diff(before, after);
    }
    printf("[CLIENT]: %d loop average nsec %lu\n", nloop, elapse/nloop);
    close(socket_fd);
    return 0;
}
