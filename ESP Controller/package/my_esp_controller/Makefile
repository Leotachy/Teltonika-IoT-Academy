include $(TOPDIR)/rules.mk

PKG_NAME:=my_esp_controller
PKG_RELEASE:=1
PKG_VERSION:=1.0.0

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

CMAKE_BINARY_SUBDIR:=build

define Package/my_esp_controller
  CATEGORY:=Base system
  TITLE:=my_esp_controller
  DEPENDS:=+libserialport +tuya_iot_core_sdk +libubus +libubox +libuci
endef

define Package/my_esp_controller/description
	A UBUS service exposing methods to control esp devices
endef

define Package/my_esp_controller/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/$(CMAKE_BINARY_SUBDIR)/src/my_esp_controller $(1)/usr/bin
	$(INSTALL_BIN) ./files/my_esp_controller.init $(1)/etc/init.d/my_esp_controller
endef

$(eval $(call BuildPackage,my_esp_controller))
