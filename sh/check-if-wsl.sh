if command -v systemd-detect-virt &> /dev/null && [[ "$(systemd-detect-virt)" == "wsl" ]]
then
  echo true
else
  echo false
fi
