/****************************************************************************
** Meta object code from reading C++ file 'detectmainwidget.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../Detect/detectmainwidget.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'detectmainwidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.8.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN16DetectMainWidgetE_t {};
} // unnamed namespace


#ifdef QT_MOC_HAS_STRINGDATA
static constexpr auto qt_meta_stringdata_ZN16DetectMainWidgetE = QtMocHelpers::stringData(
    "DetectMainWidget",
    "frameReady",
    "",
    "img",
    "showFirePage",
    "showGrowthPage",
    "showIntrusionPage",
    "showSensorPage",
    "setupAllStreams",
    "setupStream",
    "const char*",
    "cctvName",
    "index"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA

Q_CONSTINIT static const uint qt_meta_data_ZN16DetectMainWidgetE[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
       7,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,   56,    2, 0x06,    1 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       4,    0,   59,    2, 0x08,    3 /* Private */,
       5,    0,   60,    2, 0x08,    4 /* Private */,
       6,    0,   61,    2, 0x08,    5 /* Private */,
       7,    0,   62,    2, 0x08,    6 /* Private */,
       8,    0,   63,    2, 0x08,    7 /* Private */,
       9,    2,   64,    2, 0x08,    8 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::QImage,    3,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 10, QMetaType::Int,   11,   12,

       0        // eod
};

Q_CONSTINIT const QMetaObject DetectMainWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<DetectCoreWidget::staticMetaObject>(),
    qt_meta_stringdata_ZN16DetectMainWidgetE.offsetsAndSizes,
    qt_meta_data_ZN16DetectMainWidgetE,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_tag_ZN16DetectMainWidgetE_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<DetectMainWidget, std::true_type>,
        // method 'frameReady'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QImage &, std::false_type>,
        // method 'showFirePage'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'showGrowthPage'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'showIntrusionPage'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'showSensorPage'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'setupAllStreams'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'setupStream'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const char *, std::false_type>,
        QtPrivate::TypeAndForceComplete<const int, std::false_type>
    >,
    nullptr
} };

void DetectMainWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<DetectMainWidget *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->frameReady((*reinterpret_cast< std::add_pointer_t<QImage>>(_a[1]))); break;
        case 1: _t->showFirePage(); break;
        case 2: _t->showGrowthPage(); break;
        case 3: _t->showIntrusionPage(); break;
        case 4: _t->showSensorPage(); break;
        case 5: _t->setupAllStreams(); break;
        case 6: _t->setupStream((*reinterpret_cast< std::add_pointer_t<const char*>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _q_method_type = void (DetectMainWidget::*)(const QImage & );
            if (_q_method_type _q_method = &DetectMainWidget::frameReady; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
    }
}

const QMetaObject *DetectMainWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *DetectMainWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ZN16DetectMainWidgetE.stringdata0))
        return static_cast<void*>(this);
    return DetectCoreWidget::qt_metacast(_clname);
}

int DetectMainWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = DetectCoreWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 7)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 7)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 7;
    }
    return _id;
}

// SIGNAL 0
void DetectMainWidget::frameReady(const QImage & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_WARNING_POP
