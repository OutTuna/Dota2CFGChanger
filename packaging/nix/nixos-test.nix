{ pkgs, dota2cfgchanger }:
pkgs.testers.runNixOSTest {
  name = "dota2cfgchanger";
  nodes.machine = { ... }: {
    services.xserver.enable = true;
    services.xserver.desktopManager.xfce.enable = true;
    services.xserver.displayManager.lightdm.enable = true;
    services.displayManager.autoLogin = {
      enable = true;
      user = "alice";
    };
    users.users.alice = {
      isNormalUser = true;
      password = "";
    };
    hardware.graphics.enable = true;
    environment.systemPackages = [ dota2cfgchanger pkgs.xorg.xwininfo ];
    virtualisation.memorySize = 2048;
  };
  testScript = ''
    machine.start()
    machine.wait_for_x()
    machine.wait_for_file("/home/alice/.Xauthority")
    machine.succeed("test -f /run/current-system/sw/share/applications/dota2cfgchanger.desktop")
    machine.succeed("su - alice -c 'DISPLAY=:0 LIBGL_ALWAYS_SOFTWARE=1 DotaManager > /tmp/dotamanager.log 2>&1 &' ")
    machine.wait_for_window("Dota 2 CFG Changer")
    machine.sleep(3)
    machine.succeed("pgrep -u alice -f DotaManager")
    machine.screenshot("dota2cfgchanger")
    machine.succeed("pkill -u alice -f DotaManager")
  '';
}
