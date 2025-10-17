#include "slss.hh"
#include "aont.hh"

#include <cstdio>
#include <iostream>
#include <libgen.h>
#include <stdarg.h>
#include <stdlib.h>
#include <vector>

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


static void rtfm_aont(const std::string & prog, const bool copying = false)
{
  rtfm(prog, copying);
  std::cerr <<
    " " << prog << " [STUB[.aont]]\n"
    "\tSTUB           filename to encode or decode\n"
            << std::endl;
  exit(1);
}

static void rtfm_gfm(const std::string & prog, const bool copying = false)
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

static void rtfm_slss(const std::string & prog, const bool copying = false)
{
  rtfm(prog, copying);
  std::cerr <<
    " STUB [NUM_SHARES NUM_REQUIRED]\n"
    "\tSTUB           filename stub for files encrypt or recover\n"
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
  numRequired = std::stoi(args[1]);

  // limiting to 240 shares.
  // it makes no sense for only 1 share to be required to recover.
  // it also makes no sense is _all_ shares are required to recover, so
  // 2 <= numRequired < numShares <= 240
  attest((numShares >= 2) && (numShares <= 240),
         "You must specify between 3 and 240 shares");
  attest((numRequired >= 2) && (numRequired < numShares),
         "You must specify between 2 and numShares (%d) required shares", numShares);
}

int main(int argc, char ** argv)
{
  attest((argc >= 1) && (argv != NULL) && (argv[0] != NULL),
         "main(%d,%p): INVALID", argc, argv);
  const std::string prog(argv[0]);
  std::vector<std::string> args(argv + 1, argv + argc);

  const bool RunAsGFM  = ends_with(prog, "gfm");
  const bool RunAsAONT = ends_with(prog, "aont");
  const bool show      = ((args.size() == 2) &&
                          (args[0] == "show"));
  const bool copying   = (show &&
                          (*args[1].c_str() == 'c'));

  if (RunAsAONT)
  {
    if (args.size() == 0)
    {
      std::cerr << "aont encrypt STDIN to STDOUT" << std::endl;
      encrypt();
      exit(0);
    }
    if (args.size() != 1)
    {
      rtfm_aont(prog, copying);
    }
    const std::string stub(args[0]);
    if (ends_with(stub, ".aont"))
    {
      //void decrypt(const std::string & encrypted,
      const std::string plaintext(stub.substr(0,stub.size()-5));
      std::cerr << "aont decrypt " << stub << " to " << plaintext
                << std::endl;
      decrypt(stub, plaintext);
    }
    else
    {
      const std::string encrypted = stub + ".aont";
      std::cerr << "aont encrypt " << stub << " to " << encrypted
                << std::endl;
      encrypt(stub, encrypted);
    }
    exit(0);
  }
  else
  {
    if (args.size() == 1)
    {
      // recover and decrypt (unless RunAsGFM)
    }
    else if (args.size() == 3)
    {
      // encrypt (unless RunAsGFM) and split
      int numShares;
      int numRequired;
      ParseNUMs(args, numShares, numRequired);
    }
    else if (RunAsGFM)
    {
      rtfm_gfm(prog, copying);
    }
  }
  rtfm_slss(prog, copying);
}
/*
  GFM::BIT();
*/
