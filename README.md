# SysTune

SysTune is a lightweight and efficient GTK-based system settings manager built in C for Linux. It provides an all-in-one interface for managing essential system configurations while being completely **desktop-independent**. Users can effortlessly configure display settings, audio controls, connectivity options, security settings, and more, making SysTune a versatile choice for both desktop environments and lightweight window managers.

## Features
- **Display settings**: Resolution, brightness, themes
- **Audio controls**: Volume management
- **Connectivity options**: Bluetooth, WiFi
- **Autostart applications**
- **Security settings**: Firewall, SSH management
- **Default applications**
- **User permissions**: Modify group memberships
- **Custom keyboard shortcuts**: Improved workflow
- **Configuration file support**: Easy backup and restoration

## Technologies Used
- C
- GTK
- Pulseaudio
- BlueZ
- Wireless Tools
- UFW

## Usage 

### Dependencies

* gtk4 & adwaita
* nmcli
* pactl
* swww ( for wayland ) |  feh ( for xorg )
* ufw
* brightnessctl
* wlr-randr | xrandr

### Steps

1. **Clone the repo**
   ```bash
   git clone https://github.com/fulgurcode/systune.git
   ```
2. **Build**
   ```bash
   make build
   ```
3. **Run**
   ```bash
   make run
   ```

### Install

Run make install script it will install Systune system wide
   ```bash
   sudo make install
   ```

### Uninstall

To uninstall Systune just run the make uninstall script
   ```bash
   sudo make uninstall
   ```

## Contributing

Contributions are welcome! To contribute to this project:

1. **Fork the project**
2. **Clone the fork**
   ```bash
   git clone https://github.com/<username>/systune.git
   ```
3. **Add Upstream**
   ```bash
   git remote add upstream https://github.com/fulgurcode/systune.git
   ```
4. **Create a new branch**
   ```bash
   git checkout -b feature
   ```
5. **Make your changes**
6. **Commit your changes**
   ```bash
   git commit -am "Add new feature"
   ```
7. **Update main**
   ```bash
   git checkout main
   git pull upstream main
   ```
8. **Rebase to main**
   ```bash
   git checkout feature
   git rebase main
   ```
9. **Push to the branch**
   ```bash
   git push origin feature
   ```
10. **Create a new Pull Request**



## LICENSE

[The GPLv3 License (GPLv3)](LICENSE)

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.


## Contact

#### Vaishakh G K
- **Email:** [vaishakhgk2006@gmail.com](mailto:vaishakhgk2006@gmail.com)
- **Website:** [Vaishakh GK](https://vaishakhgk.com)
- **GitHub:** [VAISHAKH-GK](https://github.com/VAISHAKH-GK)

#### Shreyas S K
- **Email:** [shreyassk.dev@gmail.com](mailto:shreyassk.dev@gmail.com)
- **GitHub:** [Shreyas S K](https://github.com/shreyasskdev)
