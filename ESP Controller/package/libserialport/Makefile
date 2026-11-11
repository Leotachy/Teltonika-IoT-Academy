include $(TOPDIR)/rules.mk

PKG_NAME:=libserialport
PKG_RELEASE:=1
PKG_VERSION:=1.0.0

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=https://github.com/sigrokproject/libserialport
PKG_SOURCE_VERSION:=21b3dfe5f68c205be4086469335fd2fc2ce11ed2

include $(INCLUDE_DIR)/package.mk

define Package/libserialport
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=Lib Serial Port
endef

define Build/Configure
	( cd $(PKG_BUILD_DIR); ./autogen.sh )
	$(call Build/Configure/Default)
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/lib $(1)/usr/include/

	$(INSTALL_BIN) $(PKG_BUILD_DIR)/.libs/libserialport.so* $(1)/usr/lib/
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/libserialport.h $(1)/usr/include/
endef

define Package/libserialport/install
	$(INSTALL_DIR) $(1)/usr/lib

	$(INSTALL_BIN) $(PKG_BUILD_DIR)/.libs/libserialport.so* $(1)/usr/lib/
endef

$(eval $(call BuildPackage,libserialport))
