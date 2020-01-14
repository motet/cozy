#define CONFIG_COSE_ENCRYPT
#ifdef CONFIG_COSE_ENCRYPT

#include "cose.h"
#include "shared.h"

int cose_crypt_encipher(
        cose_crypt_context * ctx,
        const uint8_t * pld, const size_t len_pld,
        const uint8_t * tbe, const size_t len_tbe,
        const uint8_t * iv, const size_t len_iv,
        uint8_t * enc) 
{
    if (ctx->key.alg == cose_alg_aes_gcm_128 || 
            ctx->key.alg == cose_alg_aes_gcm_192 ||
            ctx->key.alg == cose_alg_aes_gcm_256) {

        if (mbedtls_gcm_crypt_and_tag(&ctx->gcm, MBEDTLS_GCM_ENCRYPT, len_pld, 
                    iv, len_iv, tbe, len_tbe, pld, enc, ctx->len_mac, enc + len_pld))
            return COSE_ERROR_ENCRYPT;

    } else return COSE_ERROR_UNSUPPORTED;
    return COSE_ERROR_NONE;
}

int cose_crypt_decipher(
        cose_crypt_context * ctx,
        const uint8_t * enc, const size_t len_enc,
        const uint8_t * tbe, const size_t len_tbe,
        const uint8_t * iv, const size_t len_iv,
        uint8_t * pld, size_t * len_pld) 
{
    if (ctx->key.alg == cose_alg_aes_gcm_128 || 
            ctx->key.alg == cose_alg_aes_gcm_192 ||
            ctx->key.alg == cose_alg_aes_gcm_256) {

        *len_pld = len_enc - ctx->len_mac;
        if (mbedtls_gcm_auth_decrypt(&ctx->gcm, *len_pld, iv, len_iv, tbe, len_tbe, 
                    enc + *len_pld, ctx->len_mac, enc, pld))
            return COSE_ERROR_DECRYPT;
            
    } else return COSE_ERROR_UNSUPPORTED;
    return COSE_ERROR_NONE;
}

int cose_crypt_init(cose_crypt_context * ctx,
        const uint8_t * key, cose_alg alg,
        const uint8_t * kid, size_t len_kid) 
{
    ctx->key.alg = alg;
    ctx->cipher = MBEDTLS_CIPHER_ID_AES;
    ctx->key.len_kid = len_kid;
    memcpy(ctx->key.kid, kid, len_kid);

    if (ctx->key.alg == cose_alg_aes_gcm_128)
        ctx->key.len_key = 16;
    else if (ctx->key.alg == cose_alg_aes_gcm_192) 
        ctx->key.len_key = 24;
    else if (ctx->key.alg == cose_alg_aes_gcm_256)
        ctx->key.len_key = 32;
    else return COSE_ERROR_UNSUPPORTED;

    if (ctx->key.alg == cose_alg_aes_gcm_128 ||
            ctx->key.alg == cose_alg_aes_gcm_192 ||
            ctx->key.alg == cose_alg_aes_gcm_256) {
        ctx->len_mac = 16;
        mbedtls_gcm_init(&ctx->gcm);
        mbedtls_gcm_setkey(&ctx->gcm, ctx->cipher, key, ctx->key.len_key * 8);
    } else return COSE_ERROR_UNSUPPORTED;

    return COSE_ERROR_NONE;
}

int cose_encrypt0_write(cose_crypt_context *ctx,
        const uint8_t * pld, size_t len_pld, 
        const uint8_t * aad, size_t len_aad,
        const uint8_t * iv, size_t len_iv,
        uint8_t * obj, size_t * len_obj) 
{
    size_t len_enc = len_pld + ctx->len_mac;
    uint8_t enc[len_enc];

    size_t len_tbe = len_aad + 32;
    uint8_t tbe[len_tbe];

    if (cose_encode_encrypt0_tbe(&ctx->key, aad, len_aad, tbe, &len_tbe)) 
        return COSE_ERROR_ENCODE;

    if (cose_crypt_encipher(ctx, pld, len_pld, tbe, len_tbe, iv, len_iv, enc))
        return COSE_ERROR_ENCRYPT;

    if (cose_encode_encrypt0_object(&ctx->key, enc, len_enc, iv, len_iv, obj, len_obj)) 
        return COSE_ERROR_ENCODE;

    return COSE_ERROR_NONE;
}

int cose_encrypt0_read(cose_crypt_context * ctx,
        const uint8_t * obj, size_t len_obj, 
        const uint8_t * aad, size_t len_aad,
        uint8_t * pld, size_t * len_pld) 
{
    size_t len_enc = len_obj;
    uint8_t enc[len_enc];

    size_t len_tbe = len_aad + 32;
    uint8_t tbe[len_tbe];

    size_t len_iv = 12;
    uint8_t iv[len_iv];

    if (cose_encode_encrypt0_tbe(&ctx->key, aad, len_aad, tbe, &len_tbe)) 
        return COSE_ERROR_ENCODE;

    if (cose_decode_encrypt0_object(obj, len_obj, enc, &len_enc, iv, &len_iv))
        return COSE_ERROR_DECODE; 

    if (cose_crypt_decipher(ctx, enc, len_enc, tbe, len_tbe, iv, len_iv, pld, len_pld))
        return COSE_ERROR_DECRYPT;

    return COSE_ERROR_NONE;
}

void cose_crypt_free(cose_crypt_context * ctx) 
{
     mbedtls_gcm_free(&ctx->gcm);
}

int cose_encode_encrypt0_tbe(
        cose_key * key,
        const uint8_t * aad, size_t len_aad,
        uint8_t * tbe, size_t * len_tbe)
{
    size_t len_pro = 8;
    uint8_t pro[len_pro];

    CborEncoder encoder_obj0, encoder_arr0;

    cbor_encoder_init(&encoder_obj0, tbe, *len_tbe, 0);
    cbor_encoder_create_array(&encoder_obj0, &encoder_arr0, 3);                 // Enc_structure
    cbor_encode_text_string(&encoder_arr0, COSE_CONTEXT_ENCRYPT0,               // context
            strlen(COSE_CONTEXT_ENCRYPT0));
    cose_encode_protected(key, pro, &len_pro);                                  // protected
    cbor_encode_byte_string(&encoder_arr0, pro, len_pro);
    cbor_encode_byte_string(&encoder_arr0, aad, len_aad);                       // external_aad
    cbor_encoder_close_container(&encoder_obj0, &encoder_arr0);

    if (cbor_encoder_get_extra_bytes_needed(&encoder_obj0)) 
        return COSE_ERROR_TINYCBOR;
    *len_tbe = cbor_encoder_get_buffer_size(&encoder_obj0, tbe);

     return COSE_ERROR_NONE;
}

int cose_encode_encrypt0_object(
        cose_key * key,
        const uint8_t * enc, size_t len_enc, 
        const uint8_t * iv, size_t len_iv,
        uint8_t * obj, size_t * len_obj) 
{
    size_t len_pro = 8;
    uint8_t pro[len_pro];

    CborEncoder encoder_obj0, encoder_arr0, 
                encoder_map0;

    cbor_encoder_init(&encoder_obj0, obj, *len_obj, 0);
    cbor_encode_tag(&encoder_obj0, cose_tag_encrypt0);                          // tag
    cbor_encoder_create_array(&encoder_obj0, &encoder_arr0, 3);
    cose_encode_protected(key, pro, &len_pro);                                  // protected
    cbor_encode_byte_string(&encoder_arr0, pro, len_pro);
    cbor_encoder_create_map(&encoder_arr0, &encoder_map0, 1);                   // unprotected
    cbor_encode_int(&encoder_map0, cose_header_iv);
    cbor_encode_byte_string(&encoder_map0, iv, len_iv);                         // iv 
    cbor_encoder_close_container(&encoder_arr0, &encoder_map0);
    cbor_encode_byte_string(&encoder_arr0, enc, len_enc);                       // ciphertext
    cbor_encoder_close_container(&encoder_obj0, &encoder_arr0);

    if (cbor_encoder_get_extra_bytes_needed(&encoder_obj0)) 
        return COSE_ERROR_TINYCBOR;
    *len_obj = cbor_encoder_get_buffer_size(&encoder_obj0, obj);

     return COSE_ERROR_NONE;
} 

int cose_decode_encrypt0_object(
        const uint8_t * obj, size_t len_obj,
        uint8_t * enc, size_t * len_enc,
        uint8_t * iv, size_t * len_iv)
{
    CborParser parser;
    CborValue par0, par1, par2;
    if (cbor_parser_init(obj, len_obj, 0, &parser, &par0) != CborNoError)
        return COSE_ERROR_TINYCBOR;
    cbor_value_skip_tag(&par0);                                      
    cbor_value_enter_container(&par0, &par1);                                   // protected
    cbor_value_advance(&par1);                                                  // unprotected 
    cbor_value_enter_container(&par1, &par2);                                   // iv
    
    int header;
    if (cbor_value_get_int(&par2, &header)) return COSE_ERROR_DECODE;
    if (header != cose_header_iv) return COSE_ERROR_DECODE;
    cbor_value_advance(&par2);
    if (cbor_value_copy_byte_string(&par2, iv, len_iv, &par2) != CborNoError)
        return COSE_ERROR_TINYCBOR;
    cbor_value_leave_container(&par1, &par2);

    if (cbor_value_copy_byte_string(&par1, enc, len_enc, &par1) != CborNoError)
        return COSE_ERROR_TINYCBOR;
    
     return COSE_ERROR_NONE;
}

#endif /* CONFIG_COSE_ENCRYPT */
