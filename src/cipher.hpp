//
// Created by ian on 5/30/24.
//

#ifndef CIPHER_HPP
#define CIPHER_HPP

#include <iostream>
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/pem.h>

#include <boost/algorithm/hex.hpp>


namespace cipher {
    inline std::mutex lock;

    inline EVP_PKEY * get_state_public_rsa() {
        std::string _path = "certificates/state.pem";
        FILE* _public_key_file = fopen(_path.c_str(), "rb");

        if (!_public_key_file) {
            throw std::runtime_error("certificates/state.pem should exists as file.");
        }

        EVP_PKEY* _public_key = PEM_read_PUBKEY(_public_key_file, nullptr, nullptr, nullptr);

        if (!_public_key)
            throw std::runtime_error("certificates/state.pem should be a valid public key.");

        fclose(_public_key_file);
        return _public_key;
    }

    inline std::string encrypt(const std::string & input) {
        EVP_PKEY * _public_key = get_state_public_rsa();
        EVP_PKEY_CTX * _context = EVP_PKEY_CTX_new(_public_key, nullptr);

        if (!_context)
            throw std::runtime_error("OpenSSL can't create context.");

        if (EVP_PKEY_encrypt_init(_context) <= 0)
            throw std::runtime_error("OpenSSL can't init the encryption.");

        size_t _cipher_text_length = 0;

        if (EVP_PKEY_encrypt(_context, nullptr, &_cipher_text_length, (const unsigned char *)input.c_str(), input.length()) <= 0)
            throw std::runtime_error("OpenSSL can't encrypt.");

        std::string _cipher_text(_cipher_text_length, '\0');

        if (EVP_PKEY_encrypt(_context, (unsigned char *) _cipher_text.data(), &_cipher_text_length, (const unsigned char *)input.c_str(), input.length()) <= 0)
            throw std::runtime_error("OpenSSL can't encrypt.");

        std::lock_guard scoped_lock(lock);
        EVP_PKEY_CTX_free(_context);

        return boost::algorithm::hex(_cipher_text);
    }
}



#endif //CIPHER_HPP
