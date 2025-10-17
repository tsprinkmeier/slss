#include "slss.hh"

#include "aont.hh"
#include "gfm.hh"

#include <cstdio>
#include <iostream>
#include <libgen.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <vector>

static void rtfm_aont(const std::string & prog, const bool copying = false)
  __attribute__((noreturn));
static void rtfm_gfm(const std::string & prog, const bool copying = false)
  __attribute__((noreturn));
static void rtfm_slss(const std::string & prog, const bool copying = false)
  __attribute__((noreturn));


bool ends_with(std::string const & str,
               std::string const & end)
{
  return ((end.size() <= str.size())
          &&
          std::equal(end.rbegin(), end.rend(), str.rbegin()));
}

static void rtfm(const std::string & prog, const bool copying = false)
{
  std::cerr <<
    "     " << prog << "  Copyright (C) 2025  Thomas Sprinkmeier\n\n";
  if (copying)
  {
    std::cerr <<
      "    This program is free software: you can redistribute it and/or modify\n"
      "    it under the terms of the GNU General Public License as published by\n"
      "    the Free Software Foundation, either version 3 of the License, or\n"
      "    (at your option) any later version.\n";
  }
  else
  {
    std::cerr <<
      "    This program is distributed in the hope that it will be useful,\n"
      "    but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
      "    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
      "    GNU General Public License for more details.\n";
  }
  std::cerr <<
    "\n"
    "    You should have received a copy of the GNU General Public License\n"
    "    along with this program.  If not, see <https://www.gnu.org/licenses/>.\n"
    "\n"
    "    run \"" << prog << " show [warranty|copying]\" for details.\n"
            << std::endl;
}


static void rtfm_aont(const std::string & prog, const bool copying)
{
  rtfm(prog, copying);
  std::cerr <<
    " " << prog << " [STUB[.aont]]\n"
    "\tSTUB           filename to encode or decode\n"
            << std::endl;
  exit(1);
}

static void rtfm_gfm(const std::string & prog, const bool copying)
{
  rtfm(prog, copying);
  std::cerr <<
    " STUB [NUM_SHARES NUM_REQUIRED]\n"
    "\tSTUB           filename stub for files to split or recover\n"
    "\tNUM_SHARES     number of shares to create\n"
    "\tNUM_REQUIRED   number of shares required to recover\n"
            << std::endl;
  exit(1);
}

static void rtfm_slss(const std::string & prog, const bool copying)
{
  rtfm(prog, copying);
  std::cerr <<
    " STUB [NUM_SHARES NUM_REQUIRED]\n"
    "\tSTUB           filename stub for files to encrypt and split\n"
    "\t               or recover and decrypt\n"
    "\tNUM_SHARES     number of shares to create\n"
    "\tNUM_REQUIRED   number of shares required to recover\n"
            << std::endl;
  exit(1);
}

void attest(bool test, const char * epilogue, ...)
{
  if (test)
  {
    return;
  }
  va_list ap;
  va_start(ap, epilogue);
  fprintf(stderr, "\n");
  vfprintf(stderr, epilogue, ap);
  fprintf(stderr, "\n");
  va_end(ap);
  exit(1);
}

static void ParseNUMs(const std::vector<std::string> & args,
                      int & numShares,
                      int & numRequired)
{
  attest(args.size() == 3, "expecting 3 arguments");
  numShares   = std::stoi(args[1]);
  numRequired = std::stoi(args[2]);

  // limiting to 240 shares.
  // it makes no sense for only 1 share to be required to recover.
  // it also makes no sense is _all_ shares are required to recover, so
  // 2 <= numRequired < numShares <= 240
  attest((numShares >= 2) && (numShares <= 240),
         "You must specify between 3 and 240 shares");
  attest((numRequired >= 2) && (numRequired < numShares),
         "You must specify between 2 and numShares (%d) required shares", numShares);
}

static void run_aont(const std::vector<std::string> & args)
{
  // streaming STDIN to STDOUT?
  const bool stream = ((args.size()  == 0) ||
                       ((args.size() == 1) &&
                        (args[0]     == "-")));
  if (stream)
  {
    std::cerr << "encrypt STDIN to STDOUT" << std::endl;
    encrypt();
    exit(0);
  }

  // if not streaming we expect a single parameter
  if (args.size() != 1)
  {
    return;
  }

  const std::string stub(args[0]);
  // determine mode based on provided filename
  if (ends_with(stub, ".aont"))
  {
    const std::string plaintext(stub.substr(0, stub.size()-5));
    std::cerr << "decrypt " << stub << " to " << plaintext
              << std::endl;
    decrypt(stub, plaintext);
    exit(0);
  }

  const std::string encrypted = stub + ".aont";
  if (access(stub.c_str(), R_OK))
  {
    std::cerr << "encrypt " << stub << " to " << encrypted
              << std::endl;
    encrypt(stub, encrypted);
    exit(0);
  }

  std::cerr << "encrypt STDIN to " << encrypted
            << std::endl;
  encrypt(STDIN_FILENO, encrypted);
  exit(0);
}

int main(int argc, char ** argv)
{
  attest((argc >= 1) && (argv != NULL) && (argv[0] != NULL),
         "main(%d,%p): INVALID", argc, argv);

  // extract program name and arguments
  const std::string prog(argv[0]);
  const std::vector<std::string> args(argv + 1, argv + argc);

  // mode (gfm, aont, slss or show license)
  const bool RunAsGFM  = ends_with(prog, "gfm");
  const bool RunAsAONT = ends_with(prog, "aont");
  const bool show      = ((args.size() == 2) &&
                          (args[0] == "show"));
  const bool copying   = (show &&
                          (*args[1].c_str() == 'c'));

  // AONT mode?
  if (RunAsAONT)
  {
    run_aont(args);
    rtfm_aont(prog, copying);
  }

  // not AONT mode, so it's either SLSS or GFM
  // single parameter is recovery mode
  if (args.size() == 1)
  {
    const std::string stub(args[0]);
    if (RunAsGFM)
    {
      std::cerr << "recovering " << stub  << std::endl;
      RecoverData(stub);
    }
    else
    {
      attest(ends_with(stub, ".aont"), "expected stub to end with \".aont\"");
      const std::string plaintext(stub.substr(0, stub.size()-5));
      std::cerr << "recovering and decrypting " << plaintext
                << std::endl;
      RecoverData(stub);
      decrypt(stub, plaintext);
    }
    exit(0);
  }

  // three parameters is generation mode
  if (args.size() == 3)
  {
    const std::string stub(args[0]);

    // parse and check number of shares and number required
    int numShares;
    int numRequired;
    ParseNUMs(args, numShares, numRequired);
    // conmvert to number of data and number of parity
    const uint8_t numData   = numRequired;
    const uint8_t numParity = numShares - numRequired;

    if (RunAsGFM)
    {
      std::cerr << "split " << stub << " into " << numShares
                << " shares of which " << numRequired
                << " are required to recover "
                << std::endl;
      CreateParity(numData, numParity, stub);
    }
    else
    {
      const std::string encrypted = stub + ".aont";
      if (access(stub.c_str(), R_OK))
      {
        std::cerr << "split " << stub << " into " << numShares
                  << " encrypted shares of which " << numRequired
                  << " are required to recover "
                  << std::endl;
        encrypt(stub, encrypted);
      }
      else
      {
        std::cerr << "split STDIN into " << numShares
                  << " encrypted shares of which " << numRequired
                  << " are required to recover "
                  << std::endl;
        encrypt(STDIN_FILENO, encrypted);
      }
      CreateParity(numData, numParity, encrypted);
    }
    exit(0);
  }

  // wrong number of arguments...
  if (RunAsGFM)
  {
    rtfm_gfm(prog, copying);
  }

  rtfm_slss(prog, copying);
}
