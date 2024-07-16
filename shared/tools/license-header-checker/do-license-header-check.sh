#!/bin/sh

: '
/*
 * Copyright 2022-2024 openDAQ d.o.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
'

# the main function
check_or_fix()
{
  if [[ "$fixargs" == "" ]]; then
    echo -e "\n${GRN}Checking license headers ...${NC}\n"
  else
    echo -e "\n${GRN}Fixing license headers ...${NC}\n"
  fi
  filter="$(cat ./ignore-filter.txt | tr '\n' ',' | tr -d '\r')"
  ./bin/license-header-checker -v $fixargs -i "$filter" "license.in" "../../../" h cs > $outFile
  if [[ $? != 0 ]]; then echo -e "\n${RED}Please use option --install or -i first${NC}"; return 0; fi
  echo -e "${YLW}> $outFile${NC}"

  echo -e "\n${GRN}Showing result ...${NC}\n"
  cat $outFile
}

# constants
RED='\033[0;31m'
GRN='\033[0;32m'
YLW='\033[0;33m'
NC='\033[0m' # No Color

# preset
fixargs=""
outFile=""

# command line argument (only one supported at a time)
case $1 in

  "--help" | "-h")
    echo -e "\n${GRN}Usage:${NC}"
    echo -e "${GRN}  --help, -h:    this usage description${NC}"
    echo -e "${GRN}  --install, -i: locally install the license-header-checker tool into ./bin/${NC}"
    echo -e "${GRN}  --check, -c:   check the files for missing or differing license comment${NC}"
    echo -e "${GRN}  --fix, -f, -r: fix the files with missing or differing license comment${NC}"
    echo -e "${GRN}  <none>         same as --check${NC}"
    ;;

  "--install" | "-i")
    echo -e "\n${GRN}Install lluissm/license-header-checker into ./bin/ ...${NC}\n" 
    curl -s https://raw.githubusercontent.com/lluissm/license-header-checker/master/install.sh | bash
    ;;

  "" | "--check" | "-c")
    outFile="./license-header-check.log"
    check_or_fix
    ;;

  "--fix" | "-f" | "-r")
    fixargs="-a -r"
    outFile="./license-header-fix.log"
    check_or_fix
    ;;

  *)
    echo -e "\n${RED}Unknown switch '$1'${NC}"
    echo -e "${YLW}Use --help or -h for usage information${NC}\n"
    exit
    ;;

esac

echo -e "\n${GRN}finished.${NC}\n"
