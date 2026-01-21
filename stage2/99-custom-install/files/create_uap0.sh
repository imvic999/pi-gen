#!/bin/bash


# 1. 等待實體網卡 wlan0 真正出現在系統中 (最多等 10 秒)
MAX_RETRIES=10
COUNT=0
while ! iw dev wlan0 info > /dev/null 2>&1; do
    echo "等待 wlan0 準備中... ($COUNT/$MAX_RETRIES)"
    sleep 1
    ((COUNT++))
    if [ $COUNT -ge $MAX_RETRIES ]; then
        echo "錯誤: 找不到實體網卡 wlan0"
        exit 1
    fi
done

# 2. 嘗試刪除舊的 uap0，並把報錯丟進黑洞 (避免日誌出現 Error -19)
iw dev uap0 del > /dev/null 2>&1

# 3. 建立虛擬介面 uap0
if iw dev wlan0 interface add uap0 type __ap; then
    echo "成功建立 uap0 介面"
else
    echo "建立 uap0 失敗"
    exit 1
fi

# 4. 給核心一點點時間反應，然後啟用它
sleep 1
if ip link set uap0 up; then
    echo "uap0 已啟用 (UP)"
else
    echo "啟用 uap0 失敗"
    exit 1
fi

# 5. 設定靜態 IP (如果您之前是透過 NetworkManager 設定，這行可以選用)
ip addr add 10.0.0.1/24 dev uap0
