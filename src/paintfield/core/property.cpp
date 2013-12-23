#include "property.h"
#include "property_p.h"
#include <QMetaProperty>

namespace PaintField {

namespace PropertyDetail {

BindObject::BindObject(const SP<Property> &p1, const SP<Property> &p2, Mode mode) :
	mProperty({{p1, p2}})
{
	auto getSlot = [](const char *name) {
		// SLOT macro prepends "1" to method name
		return staticMetaObject.method(staticMetaObject.indexOfSlot(name + 1));
	};
	if (mode == ModeDoubly) {
		if (p1->isNotifiable()) {
			connect(p1->object(), p1->notifySignal(), this, getSlot(SLOT(on1Changed())));
		}
	}
	if (p2->isNotifiable()) {
		connect(p2->object(), p2->notifySignal(), this, getSlot(SLOT(on2Changed())));
	}
	p1->set(p2->get());

	for (const auto p : mProperty) {
		connect(p->object(), SIGNAL(destroyed()), this, SLOT(deleteBinding()));
	}
}

void BindObject::deleteBinding()
{
	this->deleteLater();
	for (const auto p : mProperty) {
		disconnect(p->object(), nullptr, this, nullptr);
	}
}

BindTransformObject::BindTransformObject(const SP<Property> &property1, const SP<Property> &property2,
		const PropertyDetail::Transform &to1, const PropertyDetail::Transform &to2, Mode mode) :
	BindObject(property1, property2, mode)
{
	mTo[Channel1] = to1;
	mTo[Channel2] = to2;
	onChanged(Channel2);
}

void BindTransformObject::onChanged(Channel ch)
{
	auto value = property(ch)->get();
	auto other = opposite(ch);
	if (mValue[ch] != value) {
		mValue[ch] = value;
		mValue[other] = mTo[other](value);
		property(other)->set(mValue[other]);
	}
}

} // namespace PropertyDetail

Property::Connection::Connection(QObject *object) :
	mObject(object)
{}

void Property::Connection::disconnect()
{
	delete mObject;
	mObject = nullptr;
}

Property::Connection Property::bind(const SP<Property> &p1, const SP<Property> &p2)
{
	auto c = new PropertyDetail::BindObject(p1, p2, PropertyDetail::BindObject::ModeSingly);
	return Connection(c);
}

Property::Connection Property::bind(QObject *object1, const QByteArray &propertyName1, QObject *object2, const QByteArray &propertyName2)
{
	auto c = new PropertyDetail::BindObject(
		qtProperty(object1, propertyName1),
		qtProperty(object2, propertyName2),
		PropertyDetail::BindObject::ModeSingly);
	return Connection(c);
}

Property::Connection Property::bind(const SP<Property> &p1, const PropertyDetail::Transform &transformTo1, const SP<Property> &p2)
{
	auto c = new PropertyDetail::BindTransformObject(
		p1, p2,
		transformTo1,
		std::function<QVariant (const QVariant &)>(),
		PropertyDetail::BindObject::ModeSingly);
	return Connection(c);
}

Property::Connection Property::sync(const SP<Property> &p1, const SP<Property> &p2)
{
	auto c = new PropertyDetail::BindObject(p1, p2);
	return Connection(c);
}

Property::Connection Property::sync(QObject *object1, const QByteArray &propertyName1, QObject *object2, const QByteArray &propertyName2)
{
	auto c = new PropertyDetail::BindObject(qtProperty(object1, propertyName1), qtProperty(object2, propertyName2));
	return Connection(c);
}

Property::Connection Property::sync(const SP<Property> &p1, const PropertyDetail::Transform &transformTo1,
	const SP<Property> &p2, const PropertyDetail::Transform &transformTo2)
{
	auto c = new PropertyDetail::BindTransformObject(p1, p2, transformTo1, transformTo2);
	return Connection(c);
}

bool Property::isNotifiable() const
{
	return hasGetter() && mObject && mNotifySignal.isValid() && mNotifySignal.methodType() == QMetaMethod::Signal;
}

QtProperty::QtProperty(QObject *object, const QByteArray &propertyName)
{
	auto m = object->metaObject();
	mProperty = m->property(m->indexOfProperty(propertyName.data()));
	if (!mProperty.isValid()) {
		PAINTFIELD_WARNING << "property" << propertyName << "not found";
	}
	setNotify(object, mProperty.notifySignal());
}

CustomProperty::CustomProperty(const PropertyDetail::Setter &setter, const PropertyDetail::Getter &getter,
	QObject *object, const QMetaMethod &notifySignal) :
	mSetter(setter),
	mGetter(getter)
{
	setNotify(object, notifySignal);
}

} // namespace PaintField
