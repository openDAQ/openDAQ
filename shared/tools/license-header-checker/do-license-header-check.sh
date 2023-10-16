#!/bin/sh

: '
/*
 * Blueberry d.o.o. ("COMPANY") CONFIDENTIAL
 * Unpublished Copyright (c) 2022-2023 Blueberry d.o.o., All Rights Reserved.
 *
 * NOTICE:  All information contained herein is, and remains the property of
 * COMPANY. The intellectual and technical concepts contained herein are
 * proprietary to COMPANY and are protected by copyright law and as trade
 * secrets and may also be covered by U.S. and Foreign Patents, patents in
 * process, etc.
 * Dissemination of this information or reproduction of this material is
 * strictly forbidden unless prior written permission is obtained from COMPANY.
 * Access to the source code contained herein is hereby forbidden to anyone
 * except current COMPANY employees, managers or contractors who have executed
 * Confidentiality and Non-disclosure agreements explicitly covering such
 * access.
 *
 * The copyright notice above does not evidence any actual or intended
 * publication or disclosure  of  this source code, which includes information
 * that is confidential and/or proprietary, and is a trade secret of COMPANY.
 * ANY REPRODUCTION, MODIFICATION, DISTRIBUTION, PUBLIC PERFORMANCE, OR PUBLIC
 * DISPLAY OF OR THROUGH USE OF THIS SOURCE CODE WITHOUT THE EXPRESS
 * WRITTEN CONSENT OF COMPANY IS STRICTLY PROHIBITED, AND IN VIOLATION OF
 * APPLICABLE LAWS AND INTERNATIONAL TREATIES. THE RECEIPT OR POSSESSION OF
 * THIS SOURCE CODE AND/OR RELATED INFORMATION DOES NOT CONVEY OR IMPLY ANY
 * RIGHTS TO REPRODUCE, DISCLOSE OR DISTRIBUTE ITS CONTENTS, OR TO MANUFACTURE,
 * USE, OR SELL ANYTHING THAT IT  MAY DESCRIBE, IN WHOLE OR IN PART.
 *
 * The version is a pre-released beta meant for test purposes only
 * and is not designated for use with any other purpose.
 * COMPANY does not give any warranties regarding the absence of possible
 * errors, bugs, as well as guarantees of fitting otherwise as specified herein.
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
  ./bin/license-header-checker -v $fixargs -i "$filter" "../../../LICENSE.txt" "../../../" h > $outFile
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
