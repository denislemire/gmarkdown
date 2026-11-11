PORTNAME=	gmarkdown
DISTVERSION=	0.1.0
CATEGORIES=	deskutils gnome

MAINTAINER=	denis@generic.business
COMMENT=	GitHub-style Markdown viewer for GNOME
WWW=		https://generic.business

LICENSE=	BSD2CLAUSE
LICENSE_FILE=	${WRKSRC}/LICENSE

LIB_DEPENDS=	libwebkitgtk-6.0.so:www/webkit2-gtk@60 \
		libcmark-gfm.so:textproc/cmark-gfm

USES=		desktop-file-utils gnome meson pkgconfig
USE_GNOME=	glib20 gtk40 libadwaita

.include <bsd.port.mk>
