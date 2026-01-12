#!/bin/bash

FAIL_RESULT="===== failed"
OK_RESULT="===== ok"

otp_eth_mac=
otp_wifi_mac=
otp_bt_mac=
lower_oui=
flash_mac_type=
flash_mac=
oui=
otpMode=0
dbgMode=0

 get_eth_mac_cmd=0x00030082
get_wifi_mac_cmd=0x00030083
  get_bt_mac_cmd=0x00030084

 set_eth_cmd=0x00038082
set_wifi_cmd=0x00038083
  set_bt_cmd=0x00038084

function parse_arg()
{
    if [ "$1" == "eth" ]; then
	flash_mac_type=eth
    elif [ "$1" == "wifi" ]; then
	flash_mac_type=wifi
    elif [ "$1" == "bt" ]; then
	flash_mac_type=bt
    else
	echo $FAIL_RESULT
	echo "error mac_type=$1, should be eth/wifi/bt"
	exit 1;
    fi

    if [ ${#2} -ne 12 ]; then
	echo $FAIL_RESULT
	echo "length of MAC(${2}) must be 12"
	exit 1
    fi

    mac_pattern="[0-9a-fA-F]{12}$"
    #if ! [[ ${2} =~ $mac_pattern ]]; then
    if [[ ${2} == "000000000000" ]] || ! [[ ${2} =~ $mac_pattern ]]; then
	echo $FAIL_RESULT
	echo "MAC格式錯誤: ${2}"
	exit 1
    fi
    flash_mac=${2}
    flash_mac=${flash_mac,,}

    oui_pattern="^[oO][uU][iI]=[0-9a-fA-F]{6}$"
    for arg in "${@:3}"; do
	if [ "$arg" == "dbg" ]; then
	    dbgMode=1
	else
	    if [ "$arg" == "otp" ]; then
		otpMode=1
	    else 
		if [[ "$arg" =~ $oui_pattern ]]; then
		    oui="${arg: -6}"
		else
		    echo $FAIL_RESULT
		    echo "錯誤參數!! : $arg"
		    exit 1
		fi
	    fi
	fi
    done

    if [ "$oui" != "" ]; then
	oui=${oui,,} 
	pre_mac="${flash_mac:0:6}"

	if [ $pre_mac != $oui ]; then
	    echo $FAIL_RESULT
	    echo "OUI不匹配: $pre_mac , $oui"
	    exit 1
	fi
	echo oui=$oui
    fi
}

final_mac=
function get_mac_from_str()
{
    final_mac=
    read -a parts <<< "${1}"
    val1=${parts[5]#0x}  # d49e0500
    val2=${parts[6]#0x}  # 0000f6e5
    mac_part1="${val1:6:2}${val1:4:2}${val1:2:2}${val1:0:2}" # 00059ed4
    mac_part2="${val2:6:2}${val2:4:2}"                     # e5f6
    final_mac="${mac_part1}${mac_part2}"
}

function get_otp_mac()
{
    #get eth mac
    otp_eth_mac=$(vcmailbox $get_eth_mac_cmd 6 6 0 0)
    if [ $? -ne 0 ];then
	echo $FAIL_RESULT
	echo "get_otp_eth_mac cmd: vcmailbox $get_eth_mac_cmd 6 6 0 0"
	exit 1
    fi
    get_mac_from_str "$otp_eth_mac"
    otp_eth_mac=$final_mac
    echo otp_eth_mac=$otp_eth_mac

    #get wifi mac
    otp_wifi_mac=$(vcmailbox $get_wifi_mac_cmd 6 6 0 0)
    if [ $? -ne 0 ];then
	echo $FAIL_RESULT
	echo "get_otp_wifi_mac cmd: vcmailbox $get_wifi_mac_cmd 6 6 0 0"
	echo otp_wifi_mac=$otp_wifi_mac
	exit 1
    fi
    get_mac_from_str "$otp_wifi_mac"
    otp_wifi_mac=$final_mac
    echo otp_wifi_mac=$otp_wifi_mac

    #get bt mac
    otp_bt_mac=$(vcmailbox $get_bt_mac_cmd 6 6 0 0)
    if [ $? -ne 0 ];then
	echo $FAIL_RESULT
	echo "otp_bt_mac cmd: vcmailbox $get_bt_wifi_mac_cmd 6 6 0 0"
	exit 1
    fi
    get_mac_from_str "$otp_bt_mac"
    otp_bt_mac=$final_mac
    echo otp_bt_mac=$otp_bt_mac
}

function check_mac()
{
    if [ "$flash_mac" == "$otp_eth_mac" ]; then
	if [ "$flash_mac_type" == "eth" ]; then
	    echo $OK_RESULT
	    echo "已經燒錄過eth mac: $flash_mac"
	    exit 0
	else
	    echo $FAIL_RESULT
	    echo "無法燒錄$flash_mac_type MAC $flash_mac，與eth_mac衝突"
	    exit 1
	fi
    fi

    if [ "$flash_mac" == "$otp_wifi_mac" ]; then
	if [ "$flash_mac_type" == "wifi" ]; then
	    echo $OK_RESULT
	    echo "已經燒錄過wifi mac: $flash_mac"
	    exit 0
	else
	    echo $FAIL_RESULT
	    echo "無法燒錄$flash_mac_type MAC $flash_mac，與wifi_mac衝突"
	    exit 1
	fi
    fi

    if [ "$flash_mac" == "$otp_bt_mac" ]; then
	if [ "$flash_mac_type" == "bt" ]; then
	    echo $OK_RESULT
	    echo "已經燒錄過bt mac: $flash_mac"
	    exit 0
	else
	    echo $FAIL_RESULT
	    echo "無法燒錄$flash_mac_type MAC $flash_mac，與bt_mac衝突"
	    exit 1
	fi
    fi
}

############### MAIN ###############

if [ "$#" -lt 2 ]; then
    echo $FAIL_RESULT
    echo "usage: set_mac.sh eth/wifi/bt mac [OUI=XXXXXX] [dbg]"
    exit 1
fi

parse_arg $@

get_otp_mac

check_mac 

input_param_lower="${2,,}"
b1=${input_param_lower:0:2}
b2=${input_param_lower:2:2}
b3=${input_param_lower:4:2}
b4=${input_param_lower:6:2}
b5=${input_param_lower:8:2}
b6=${input_param_lower:10:2}

#echo b1=$b1
#echo b2=$b2
#echo b3=$b3
#echo b4=$b4
#echo b5=$b5
#echo b6=$b6

row1=0x$b4$b3$b2$b1
row0=0x$b6$b5

#check mac format
check_mac_cmd="vcmailbox 0x00030085 6 6 $row1 $row0"
$check_mac_cmd
last_line=$(sudo vclog -m | tail -n 2 | head -n 1)
mac_address=$(echo "$last_line" | awk '{print $NF}')
echo mac_address=$mac_address
if [ "$mac_address" != "$b1:$b2:$b3:$b4:$b5:$b6" ]; then
    echo $FAIL_RESULT
    echo "org_mac=$b1:$b2:$b3:$b4:$b5:$b6 and mac_address=$mac_address"
    exit 1
fi

exit 1

set_mac_cmd="vcmailbox $otp 6 6 $row1 $row0"
if [ "$otpMode" == "1" ]; then
    #echo "need to execute $set_mac_cmd"
    $set_mac_cmd
else
    echo "donot execute $set_mac_cmd"
fi
echo $OK_RESULT
echo "set_mac_cmd=$set_mac_cmd"
echo "correct mac format"
exit 0
