task default: :build

desc "Build R2P2-nRF52 firmware"
task :build do
  sh "make build-cdc-dual"
end

desc "Clean build outputs"
task :clean do
  sh "make clean"
end

desc "Flash the current UF2 to NRF52BOOT"
task :flash do
  sh "cp -X build/ssci_isp1807_dev_board/firmware.uf2 /Volumes/NRF52BOOT/"
end
