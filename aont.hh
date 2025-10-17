#pragma once
#include <string>

// OPENSSL_USER_MACROS(7SSL)
#define OPENSSL_NO_DEPRECATED
#include <openssl/evp.h>
#include <openssl/rand.h>

/// decrypt
void decrypt(const std::string & encrypted,
             const std::string & plaintext,
             const EVP_MD      * md = nullptr,
             const EVP_CIPHER  * cipher = nullptr,
             ENGINE            * engine = nullptr);
// encrypt
void encrypt(const std::string & plaintext,
             const std::string & encrypted,
             const EVP_MD      * md = nullptr,
             const EVP_CIPHER  * cipher = nullptr,
             ENGINE            * engine = nullptr);
// encrypt given file descriptor to given file
void encrypt(int fdIn,
             const std::string & encrypted,
             const EVP_MD      * md = nullptr,
             const EVP_CIPHER  * cipher = nullptr,
             ENGINE            * engine = nullptr);
// encrypt STDIN to STDOUT
void encrypt(const EVP_MD      * md = nullptr,
             const EVP_CIPHER  * cipher = nullptr,
             ENGINE            * engine = nullptr);

// forward declatarion to allow friend-ing.
class Digest2;

/// Cryptographic digest
class Digest
{
public:
  Digest(const EVP_MD * _type, ENGINE * _impl);
  virtual ~Digest();
  void update(const std::string & buff);
  void update(const void * buff, size_t len);
  virtual std::string final();
  std::string final(const std::string & buff);
  std::string final(const void * buff, size_t len);
  virtual void reset();
  size_t length();

private:
  void Create();
  void Destroy();
  EVP_MD_CTX   * ctx;
  const EVP_MD * type;
  ENGINE       * impl;
  friend class Digest2;
};

/// H2(x) = H(H(0 + x))
class Digest2 : public Digest
{
public:
  Digest2(const EVP_MD * _type, ENGINE * _impl);
  virtual std::string final() override;
  void reset() override;
};

class Cipher
{
public:
  Cipher(const EVP_CIPHER *_type, ENGINE *_impl);
  virtual ~Cipher();
  size_t keyLength();
  size_t ivLength();

protected:
  EVP_CIPHER_CTX   * ctx;
  const EVP_CIPHER * type;
  ENGINE           * impl;
};

/// Encryptor
class Encrypter : public Cipher
{
public:
  Encrypter(const EVP_CIPHER *_type, ENGINE *_impl);
  virtual ~Encrypter();
  const std::string & getKey();
  const std::string & getIV();
  std::string update(const std::string & buff);
  std::string final();
private:
  std::string key;
  std::string iv;
};

/// read from given file descryptor, encrpting along the way
class EncryptingReader
{
public:
  EncryptingReader(const int _fd,
                   const EVP_MD     * md     = nullptr,
                   const EVP_CIPHER * cipher = nullptr,
                   ENGINE           * engine = nullptr);
  ssize_t readFully(void * pBuff, const ssize_t len);
private:
  int fd;
  bool eof;
  std::string cache;
  Digest2      digest;
  Encrypter   encrypter;
};
