#include "aont.hh"
#include "slss.hh"

#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

const size_t       BUFF_SIZE = 4096;
const EVP_MD     * DEFAULT_MD_type = nullptr;
const EVP_CIPHER * DEFAULT_CIPHER  = nullptr;
ENGINE           * DEFAULT_ENGINE  = nullptr;

#define DUMP(x) dump(#x, x)

/**
 * @brief Dump the string to STDOUT if ${DBG}
 *
 * @param tag tag for the dump
 * @param buff buffer to dump as hex
 */
static void dump(const std::string & tag, const std::string & buff)
{
  static const bool dbg = getenv("DBG");
  if (!dbg) return;

  std::ios  state(NULL);
  state.copyfmt(std::cerr);

  std::cerr << tag << "["
            << std::dec << buff.length()
            << "]" << std::endl;
  for (unsigned i = 0; i < buff.length(); ++i)
  {
    std::cerr << std::setw(2) << std::setfill('0')
              << std::hex     << (buff[i] & 0xff);
  }
  std::cerr << std::endl;
  std::cerr.copyfmt(state);
}

/**
 * @brief Fill str with cryptographically random data.
 *
 * @param str Buffer to fill
 */
static void randomise(std::string & str)
{
  const int rc = RAND_bytes((unsigned char *)&str[0], str.length());
  attest(rc == 1, "RAND_bytes() failed");
}

/**
 * @brief Fidd str with cryptographically random data.
 *
 * @param str Buffer to fill
 * @param len Size of buffer
 */
static void randomise(std::string & str, size_t len)
{
  str.resize(len);
  randomise(str);
}

/**
 * @brief return byte-wise XOR of a and b.
 *
 * @param a
 * @param b
 *
 * @return a ^ b
 */
static std::string Xor(const std::string & a, const std::string & b)
{
  attest(a.length() == b.length(),
         "cannot Xor([%zu],[%zu])",
         a.length(), b.length());
  std::string ret(a);
  for (size_t i = 0; i < b.length(); ++i)
  {
    ret[i] ^= b[i];
  }
  return ret;
}

/**
 * @brief write the buffer to the given file
 *
 * @param fd file to write to
 * @param buff buffer to write
 */
static void write(int fd, const std::string & buff)
{
  size_t rc = write(fd, &buff[0], buff.length());
  attest(rc == (size_t)buff.length(),
         "write(%d,%%p,%zu): %zd (%m)",
         fd, buff.length(), rc);
}

/**
 * @brief Read from the given file descriptor
 *
 * @param fd file to read from
 * @param len number of bytes to read
 * @param exact enforce reading exactly
 *
 * @return up to len bytes, or exactly len bytes.
 */
static std::string read(int fd, size_t len, bool exact)
{
  std::string ret;
  ret.resize(len);
  errno = 0;
  const ssize_t rc = read(fd, &ret[0], ret.length());
  if (exact)
  {
    attest(rc == (ssize_t)ret.length(),
           "failed to read exactly %zu bytes, got %zd (%m)",
           ret.length(), rc);
  }
  else
  {
    attest(rc >= 0,
           "read(%d,%%p,%zu) failed: %m",
           fd, ret.length());
    ret.resize(rc);
  }
  return ret;
}

// forward declatarion to allow friend-ing. Lol.
class Digest2;

/**
 * @brief Cryptographic digest
 *
 * @param _type Hash, defaults to SHA384
 * @param _impl Implementation, default to software
 */

Digest::Digest(const EVP_MD * _type, ENGINE * _impl)
  : ctx(nullptr)
  , type(_type ? _type : EVP_sha384())
  , impl(_impl)
{
  Create();
};

Digest::~Digest()
{
  Destroy();
};

void Digest::update(const std::string & buff)
{
  update(&buff[0], buff.length());
};

void Digest::update(const void * buff, size_t len)
{
  int rc = EVP_DigestUpdate(ctx, buff, len);
  attest(rc == 1, "EVP_DigestUpdate() failed");
}

std::string Digest::final()
{
  unsigned char hash[EVP_MAX_MD_SIZE];
  unsigned int  len = sizeof(hash);

  const int rc = EVP_DigestFinal_ex(ctx, hash, &len);
  attest(rc == 1, "EVP_DigestFinal_ex() failed\n");
  attest(len <= EVP_MAX_MD_SIZE,
         "Digest too big: %u > %d", len, EVP_MAX_MD_SIZE);
  attest(len == length(), "unexpected hash length: %u != %zu",
         len, length());
  return std::string(hash, hash+len);
}

std::string Digest::final(const std::string & buff)
{
  return final(&buff[0], buff.length());
};

std::string Digest::final(const void * buff, size_t len)
{
  update(buff, len);
  return final();
};

void Digest::reset()
{
  Destroy();
  Create();
};

size_t Digest::length()
{
  return EVP_MD_size(type);
};

void Digest::Create()
{
  ctx = EVP_MD_CTX_create();
  attest(ctx != nullptr, "EVP_MD_CTX_create() failed");
  const int rc = EVP_DigestInit_ex(ctx, type, impl);
  attest(rc == 1, "EVP_DigestInit_ex() failed");
};

void Digest::Destroy()
{
  EVP_MD_CTX_destroy(ctx);
};

/**
 * @brief Nested cryptographic hash to avoid extension attacks
 *
 * H2(x) = H(H(0 + x))
 *
 * @param _type Hash, defaults to SHA384
 * @param _impl Implementation, default to software
 */

Digest2::Digest2(const EVP_MD * _type, ENGINE * _impl)
  : Digest(_type, _impl)
{
  update(std::string(length(), '\0'));
};

std::string Digest2::final()
{
  Digest d(type, impl);
  return d.final(Digest::final());
};

void Digest2::reset()
{
  Digest::reset();
  update(std::string(length(), '\0'));
};


/**
 * @brief Stream cipher
 *
 * @param _type Cipher, defaults to AES-256-CBC
 * @param _impl Implementation, default to software
 */

Cipher::Cipher(const EVP_CIPHER *_type, ENGINE *_impl)
  : ctx(nullptr)
  , type(_type ? _type : EVP_aes_256_cbc())
  , impl(_impl)
{
  ctx = EVP_CIPHER_CTX_new();
};
Cipher::~Cipher()
{
  EVP_CIPHER_CTX_free(ctx);
};
size_t Cipher::keyLength()
{
  return EVP_CIPHER_key_length(type);
};
size_t Cipher::ivLength()
{
  return EVP_CIPHER_iv_length(type);
};

/**
 * @brief Encryptor
 *
 * @param _type Cipher
 * @param _impl Implementation
 */
Encrypter::Encrypter(const EVP_CIPHER *_type, ENGINE *_impl)
  : Cipher(_type, _impl)
{
  randomise(key, keyLength());
  randomise(iv,  ivLength());
  dump("Key: ", key);
  dump("IV : ", iv);

  int rc = EVP_EncryptInit_ex(
    ctx, type, impl,
    (unsigned char *)&key[0],
    (unsigned char *)&iv[0]);
  attest(rc == 1, "EVP_EncryptInit_ex() fail");
};
Encrypter::~Encrypter()
{
  ;
};

const std::string & Encrypter::getKey(){return key;};
const std::string & Encrypter::getIV() {return iv; };

std::string Encrypter::update(const std::string & buff)
{
  std::string ret;
  ret.resize(buff.length() + EVP_CIPHER_block_size(type));
  int outl = ret.length();

  int rc = EVP_EncryptUpdate(
    ctx,
    (unsigned char *)&ret [0], & outl,
    (unsigned char *)&buff[0], buff.length());
  attest(rc == 1, "EVP_EncryptUpdate() fail");

  attest(outl <= (int)ret.length(),
         "EVP_EncryptUpdate() returned too much, %d > %zu",
         outl, ret.length());
  ret.resize(outl);

  return ret;
};

std::string Encrypter::final()
{
  std::string ret;
  ret.resize(EVP_CIPHER_block_size(type));
  int outl = ret.length();

  int rc = EVP_EncryptFinal_ex(
    ctx,
    (unsigned char *)&ret[0], &outl);
  attest(rc == 1, "EVP_EncryptFinal_ex() fail");

  attest(outl <= (int)ret.length(),
         "EVP_EncryptFinal_ex() returned too much, %d > %zu",
         outl, ret.length());
  ret.resize(outl);

  return ret;
};

class Decrypter : public Cipher
{
public:
  Decrypter(const std::string & key,
            const std::string & iv,
            const EVP_CIPHER *_type,
            ENGINE *_impl)
    : Cipher(_type, _impl)
    {
      init(key, iv);
    };

  Decrypter(const std::string & keyAndIV,
            const EVP_CIPHER *_type, ENGINE *_impl)
    : Cipher(_type, _impl)
    {
      const std::string iv (keyAndIV.substr(0, ivLength()));
      const std::string key(keyAndIV.substr(ivLength()));
      init(key, iv);
    };

  virtual ~Decrypter()
    {
      ;
    };

  std::string update(const std::string & buff)
    {
      std::string ret;
      ret.resize(buff.length() + EVP_CIPHER_block_size(type));
      int outl = ret.length();

      int rc = EVP_DecryptUpdate(
        ctx,
        (unsigned char *)&ret [0], & outl,
        (unsigned char *)&buff[0], buff.length());
      attest(rc == 1, "EVP_DecryptUpdate() failed");

      attest(outl <= (int)ret.length(),
             "EVP_DecryptUpdate() returned too much, %d > %zu",
             outl, ret.length());
      ret.resize(outl);
      std::cerr << "update([" << buff.size() << "]): [" << outl << "]" << std::endl;

      return ret;
    };

  std::string final()
    {
      std::string ret;
      ret.resize(EVP_CIPHER_block_size(type));
      int outl = ret.length();

      const int rc = EVP_DecryptFinal_ex(
        ctx,
        (unsigned char *)&ret[0], &outl);
      attest(rc == 1,
             "EVP_DecryptFinal_e([%d]->[%d]) fail",
             EVP_CIPHER_block_size(type), outl);

      attest(outl <= (int)ret.length(),
             "EVP_DecryptFinal_ex() block oversize: %d > %zu",
             outl, ret.length());
      ret.resize(outl);

      std::cerr << "final): [" << outl << "]" << std::endl;
      return ret;
    };
private:
  void init(const std::string & key,
            const std::string & iv)
    {
      attest(key.length() == keyLength(),
             "Unexpected key length: %zu vz %zu",
             key.length(), keyLength());
      const int rc = EVP_DecryptInit_ex(ctx, type, impl,
                                        (unsigned char *)&key[0],
                                        (unsigned char *)&iv[0]);
      attest(rc == 1, "EVP_DecryptInit_ex() failed: %d\n", rc);
    };

};

/**
 * @brief decrypt the given file to stdout
 *
 * @param filename File to decrypt
 */
void decrypt(const std::string & encrypted,
             const std::string & plaintext,
             const EVP_MD      * md,
             const EVP_CIPHER  * cipher,
             ENGINE            * engine)
{

  // open the file to decrypt
  int fd = open(encrypted.c_str(), O_RDONLY);
  attest(fd != -1, "open(%s, RDONLY): %m", encrypted.c_str());
  int fdOut = open(plaintext.c_str(),
                   O_CREAT | O_EXCL | O_RDWR,
                   S_IRUSR | S_IWUSR);
  attest(fdOut != -1, "open(%s,RDWR): %m", plaintext.c_str());

  // find out how big it is
  struct stat buf;
  int rc = fstat(fd, &buf);
  attest(rc == 0, "fstat() failed: %m");

  // how much of the file is 'data'?
  Digest2 digest(md, engine);
  size_t len = buf.st_size - digest.length();
  size_t rem = len;
  // first pass, read all the ciphertext
  while(rem)
  {
    // read up to BUFF_SIZE bytes at a time ...
    std::string buff = read(fd, std::min(rem, BUFF_SIZE), true);
    DUMP(buff);
    // redundant, read() would have attested...
    attest(buff.length(), "unexpected EOF (%zu)", rem);
    // and digest them.
    rem -= buff.length();
    digest.update(buff);
  }
  // hash of the data area
  std::string hash = digest.final();

  // now read the encrypted key and ensure it's really EOF
  std::string enc = read(fd, digest.length(), true);
  DUMP(enc);
  std::string eof = read(fd, 1, false);
  attest(eof.length() == 0, "not EOF: %zu", eof.length());

  // recover the key and the IV
  Decrypter decrypter(Xor(hash, enc), cipher, engine);
  digest.reset();

  lseek(fd, 0, SEEK_SET);
  rem = len;
  while(rem)
  {
    std::string buff = read(fd, std::min(rem, BUFF_SIZE), true);
    attest(buff.length(), "unexpected EOF2 (%zu)", rem);
    rem -= buff.length();
    digest.update(buff);
    write(fdOut, decrypter.update(buff));
  }
  write(fdOut, decrypter.final());
  close(fdOut);

  // make sure that the second read of the data has the same hash ...
  std::string hash2 = digest.final();
  attest(hash == hash2, "hash mismatch!");

  // ... and appended encrypted key
  std::string enc2 = read(fd, digest.length(), true);
  attest(enc == enc2, "enc mismatch!");

  // ensure that we're at EOF
  eof = read(fd, 1, false);
  attest(eof.length() == 0, "not EOF2: %zu", eof.length());
}

EncryptingReader::EncryptingReader(const int _fd,
                                   const EVP_MD     * md,
                                   const EVP_CIPHER * cipher,
                                   ENGINE           * engine)

  : fd(_fd)
  , eof(false)
  , digest(md, engine)
  , encrypter(cipher, engine)
{
};

ssize_t EncryptingReader::readFully(void * pBuff, const ssize_t len)
{
  // keep topping up the cache until we wither have enough or there's no more to
  while(!eof && static_cast<ssize_t>(cache.length()) < len)
  {
    // read next 4K
    std::string buff = read(fd, BUFF_SIZE, false);
    std::cerr << "readFully(*, " << len << "): ["
              << cache.length() << "] + "
              << buff.length() << std::endl;
    eof = (buff.length() == 0);
    if (eof)
    {
      fd = -1;

      std::string enc = encrypter.final();
      digest.update(enc);
      DUMP(enc);
      cache += enc;
      std::cerr << "readFully(*, " << len << ") eof: += "
                << enc.length() << std::endl;

      // now get the hash calculated for the ciphertext
      std::string hash = digest.final();

      // XOR it with the key and append
      enc = Xor(encrypter.getIV() + encrypter.getKey(), hash);
      cache += enc;
      std::cerr << "readFully(*, " << len << ") key: += "
                << enc.length() << std::endl;
      continue;
    }
    // encrypt the block ...
    std::string enc = encrypter.update(buff);
    // ... and update the hash
    digest.update(enc);
    cache += enc;
    std::cerr << "readFully(*, " << len << ") enc: += "
              << enc.length() << std::endl;
  }
  // calculate how much will be returned
  ssize_t ret = std::min(static_cast<ssize_t>(cache.length()), len);
  memcpy(pBuff, cache.c_str(), ret);
  cache = cache.substr(ret);
  return ret;
};
