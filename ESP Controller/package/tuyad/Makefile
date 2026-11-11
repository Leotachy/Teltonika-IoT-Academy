include $(TOPDIR)/rules.mk

PKG_NAME:=tuyad
PKG_RELEASE:=1
PKG_VERSION:=1.0.0

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

CMAKE_BINARY_SUBDIR:=build

define Package/tuyad
  CATEGORY:=Base system
  TITLE:=Tuyad
  DEPENDS:=+tuya_iot_core_sdk +libubus +libubox +libuci +my_esp_controller
endef

define Package/tuyad/description
	Fetches system data from the system and uploads to Tuya cloud
endef

define Package/tuyad/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/$(CMAKE_BINARY_SUBDIR)/src/tuyad $(1)/usr/bin
	$(INSTALL_BIN) ./files/tuyad.init $(1)/etc/init.d/tuyad
	$(INSTALL_CONF) ./files/tuyad.config $(1)/etc/config/tuyad
endef

$(eval $(call BuildPackage,tuyad))
