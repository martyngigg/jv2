// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2022 E. Devlin and T. Youngs

// For static Qt builds dynamic platform plugins
// are not included by default.
// Here we include the import the relevant plugin
// to force the symbols to be included

#include <QtPlugin>

// TODO: Generalize this for other platforms
Q_IMPORT_PLUGIN(QXcbIntegrationPlugin)
