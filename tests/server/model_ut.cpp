/******************************************************************************
 *   Copyright (C) 2013-2014 by Alexander Rykovanov                        *
 *   rykovanov.as@gmail.com                                                   *
 *                                                                            *
 *   This library is free software; you can redistribute it and/or modify     *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation; version 3 of the License.     *
 *                                                                            *
 *   This library is distributed in the hope that it will be useful,          *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU Lesser General Public License for more details.                      *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public License *
 *   along with this library; if not, write to the                            *
 *   Free Software Foundation, Inc.,                                          *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.                *
 ******************************************************************************/


#include <opc/ua/model.h>

#include <opc/common/addons_core/addon_manager.h>
#include <opc/ua/protocol/attribute_ids.h>
#include <opc/ua/protocol/status_codes.h>
#include <opc/ua/services/services.h>
#include <opc/ua/server/address_space.h>
#include <opc/ua/server/standard_namespace.h>

#include "address_space_registry_test.h"
#include "services_registry_test.h"
#include "standard_namespace_test.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;


class Model : public Test
{
protected:
  virtual void SetUp()
  {
    const bool debug = false;
    Addons = Common::CreateAddonsManager();

    OpcUa::Test::RegisterServicesRegistry(*Addons);
    OpcUa::Test::RegisterAddressSpace(*Addons);
    OpcUa::Test::RegisterStandardNamespace(*Addons);
    Addons->Start();

    OpcUa::Server::ServicesRegistry::SharedPtr addon = Addons->GetAddon<OpcUa::Server::ServicesRegistry>(OpcUa::Server::ServicesRegistryAddonID);
    Services = addon->GetServer();
  }

  virtual void TearDown()
  {
    Services.reset();
    Addons->Stop();
    Addons.reset();
  }

  OpcUa::NodeID CreateEmptyObjectType()
  {
    OpcUa::NodeManagementServices::SharedPtr nodes = Services->NodeManagement();
    OpcUa::AddNodesItem item;
    item.BrowseName = OpcUa::QualifiedName("empty_object_type");
    item.Class = OpcUa::NodeClass::ObjectType;
    item.ParentNodeId = OpcUa::ObjectID::BaseObjectType;
    item.ReferenceTypeId = OpcUa::ObjectID::HasSubtype;

    OpcUa::ObjectTypeAttributes attrs;
    attrs.Description = OpcUa::LocalizedText("empty_object_type");
    attrs.DisplayName = OpcUa::LocalizedText("empty_object_type");
    attrs.IsAbstract = false;
    item.Attributes = attrs;
    std::vector<OpcUa::AddNodesResult> result = nodes->AddNodes({item});
    return result[0].AddedNodeID;
  }

  OpcUa::NodeID CreateObjectTypeWithOneVariable()
  {
    const OpcUa::NodeID& objectID = CreateEmptyObjectType();
    OpcUa::AddNodesItem variable;
    variable.BrowseName = OpcUa::QualifiedName("new_variable1");
    variable.Class = OpcUa::NodeClass::Variable;
    variable.ParentNodeId = objectID;
    variable.ReferenceTypeId = OpcUa::ObjectID::HasProperty;
    OpcUa::VariableAttributes attrs;
    attrs.DisplayName = OpcUa::LocalizedText("new_variable");
    variable.Attributes = attrs;
    Services->NodeManagement()->AddNodes({variable});
    return objectID;
  }

  OpcUa::NodeID CreateObjectTypeWithOneUntypedObject()
  {
    const OpcUa::NodeID& objectID = CreateEmptyObjectType();
    OpcUa::AddNodesItem object;
    object.BrowseName = OpcUa::QualifiedName("new_sub_object1");
    object.Class = OpcUa::NodeClass::Object;
    object.ParentNodeId = objectID;
    object.ReferenceTypeId = OpcUa::ObjectID::HasComponent;
    OpcUa::ObjectAttributes attrs;
    attrs.DisplayName = OpcUa::LocalizedText("new_sub_object");
    object.Attributes = attrs;
    Services->NodeManagement()->AddNodes({object});
    return objectID;
  }

  OpcUa::NodeID CreateObjectTypeWithOneTypedObject()
  {
    const OpcUa::NodeID& resultTypeID = CreateEmptyObjectType();
    const OpcUa::NodeID& objectTypeWithVar = CreateObjectTypeWithOneVariable();
    OpcUa::AddNodesItem object;
    object.BrowseName = OpcUa::QualifiedName("new_sub_object1");
    object.Class = OpcUa::NodeClass::Object;
    object.ParentNodeId = resultTypeID;
    object.ReferenceTypeId = OpcUa::ObjectID::HasComponent;
    object.TypeDefinition = objectTypeWithVar;
    OpcUa::ObjectAttributes attrs;
    attrs.DisplayName = OpcUa::LocalizedText("new_sub_object");
    object.Attributes = attrs;
    Services->NodeManagement()->AddNodes({object});
    return resultTypeID;
  }

protected:
  Common::AddonsManager::UniquePtr Addons;
  OpcUa::Services::SharedPtr Services;
};


TEST_F(Model, ServerCanAccessToRootObject)
{
  OpcUa::Model::Server server(Services);
  OpcUa::Model::Object rootObject = server.RootObject();

  ASSERT_EQ(rootObject.GetID(), OpcUa::ObjectID::RootFolder);
}

TEST_F(Model, ObjectCanCreateVariable)
{
  OpcUa::Model::Server server(Services);
  OpcUa::Model::Object rootObject = server.RootObject();
  OpcUa::QualifiedName name("new_variable");
  OpcUa::Variant value = 8;
  OpcUa::Model::Variable variable = rootObject.CreateVariable(name, value);

  ASSERT_NE(variable.GetID(), OpcUa::ObjectID::Null);
  ASSERT_EQ(variable.GetBrowseName(), name);
  ASSERT_EQ(variable.GetDisplayName(), OpcUa::LocalizedText(name.Name));
  ASSERT_EQ(variable.GetValue(), value);
}

TEST_F(Model, CanSetVariableValue_ByVariant)
{
  OpcUa::Model::Server server(Services);
  OpcUa::Model::Object rootObject = server.RootObject();
  OpcUa::QualifiedName name("new_variable");
  OpcUa::Variant value = 8;
  OpcUa::Model::Variable variable = rootObject.CreateVariable(name, value);

  ASSERT_NE(variable.GetID(), OpcUa::ObjectID::Null);
  ASSERT_EQ(variable.GetBrowseName(), name);
  ASSERT_EQ(variable.GetDisplayName(), OpcUa::LocalizedText(name.Name));
  ASSERT_EQ(variable.GetValue(), value);

  variable.SetValue(10);
  OpcUa::DataValue data = variable.GetValue();
  ASSERT_TRUE(data.Encoding & (OpcUa::DATA_VALUE));
  ASSERT_TRUE(data.Encoding & (OpcUa::DATA_VALUE_SOURCE_TIMESTAMP));
  EXPECT_EQ(data.Value, 10);
  EXPECT_NE(data.SourceTimestamp, 0);
}

TEST_F(Model, CanSetVariableValue_DataValue)
{
  OpcUa::Model::Server server(Services);
  OpcUa::Model::Object rootObject = server.RootObject();
  OpcUa::QualifiedName name("new_variable");
  OpcUa::Variant value = 8;
  OpcUa::Model::Variable variable = rootObject.CreateVariable(name, value);

  ASSERT_NE(variable.GetID(), OpcUa::ObjectID::Null);
  ASSERT_EQ(variable.GetBrowseName(), name);
  ASSERT_EQ(variable.GetDisplayName(), OpcUa::LocalizedText(name.Name));
  ASSERT_EQ(variable.GetValue(), value);

  OpcUa::DataValue data(10);
  data.SetSourceTimestamp(OpcUa::DateTime(12345));
  variable.SetValue(data);
  OpcUa::DataValue result = variable.GetValue();
  ASSERT_TRUE(result.Encoding & (OpcUa::DATA_VALUE));
  ASSERT_TRUE(result.Encoding & (OpcUa::DATA_VALUE_SOURCE_TIMESTAMP));
  EXPECT_EQ(result.Value, 10);
  EXPECT_EQ(result.SourceTimestamp, 12345);
}

TEST_F(Model, CanInstantiateEmptyObjectType)
{
  const OpcUa::NodeID& typeID = CreateEmptyObjectType();
  OpcUa::Model::ObjectType objectType(typeID, Services);
  OpcUa::Model::Object rootObject(OpcUa::ObjectID::RootFolder, Services);
  const char* objectDesc = "Empty object.";
  const OpcUa::QualifiedName browseName("empty_object");
  const OpcUa::NodeID objectID = rootObject.CreateObject(objectType, browseName, objectDesc).GetID();
  OpcUa::Model::Object object(objectID, Services);

  ASSERT_NE(object.GetID(), OpcUa::ObjectID::Null);
  ASSERT_EQ(object.GetBrowseName(), browseName) << "Real name: " << object.GetBrowseName().Name;

  std::vector<OpcUa::Model::Variable> variables = object.GetVariables();
  ASSERT_EQ(variables.size(), 0);
}

TEST_F(Model, CanInstantiateObjectTypeWithOneVariable)
{
  const OpcUa::NodeID& typeID = CreateObjectTypeWithOneVariable();
  OpcUa::Model::ObjectType objectType(typeID, Services);
  OpcUa::Model::Object rootObject(OpcUa::ObjectID::RootFolder, Services);
  const char* objectDesc = "object_with_var.";
  const OpcUa::QualifiedName browseName("object_with_var");
  const OpcUa::NodeID objectID = rootObject.CreateObject(objectType, browseName, objectDesc).GetID();
  OpcUa::Model::Object object(objectID, Services);

  ASSERT_NE(object.GetID(), OpcUa::ObjectID::Null);
  ASSERT_EQ(object.GetBrowseName(), browseName) << "Real name: " << object.GetBrowseName().Name;

  std::vector<OpcUa::Model::Variable> variables = object.GetVariables();
  ASSERT_EQ(variables.size(), 1);
}

TEST_F(Model, CanInstantiateObjectTypeWithOneUntypedObject)
{
  const OpcUa::NodeID& typeID = CreateObjectTypeWithOneUntypedObject();
  OpcUa::Model::ObjectType objectType(typeID, Services);
  OpcUa::Model::Object rootObject(OpcUa::ObjectID::RootFolder, Services);
  const char* objectDesc = "object_with_var.";
  const OpcUa::QualifiedName browseName("object_with_var");
  const OpcUa::NodeID objectID = rootObject.CreateObject(objectType, browseName, objectDesc).GetID();
  OpcUa::Model::Object object(objectID, Services);

  ASSERT_NE(object.GetID(), OpcUa::ObjectID::Null);
  ASSERT_EQ(object.GetBrowseName(), browseName) << "Real name: " << object.GetBrowseName().Name;

  std::vector<OpcUa::Model::Object> objects = object.GetObjects();
  ASSERT_EQ(objects.size(), 1);
}

TEST_F(Model, CanInstantiateObjectTypeWithOneTypedObject)
{
  // Type with one property - empty object with type that has a variable.
  // ObjectType1
  //   +-object - ObjectType2
  //
  // ObjectType2
  //   +-variable

  const OpcUa::NodeID& typeID = CreateObjectTypeWithOneTypedObject();
  OpcUa::Model::ObjectType objectType(typeID, Services);
  // we will create objects under root folder.
  OpcUa::Model::Object rootObject(OpcUa::ObjectID::RootFolder, Services);
  const char* objectDesc = "object_with_var.";
  const OpcUa::QualifiedName browseName("object_with_var");
  // Instantiate object type we have created first.
  // Get only id of that object.
  const OpcUa::NodeID objectID = rootObject.CreateObject(objectType, browseName, objectDesc).GetID();
  // This constructor will read all parameters of created object.
  // Restored object structure should be next:
  // Object1 - ObjectType1
  //   +-Object2 - ObjectType2
  //       +-variable
  OpcUa::Model::Object object(objectID, Services);

  ASSERT_EQ(object.GetID(), objectID);
  ASSERT_EQ(object.GetBrowseName(), browseName) << "Real name: " << object.GetBrowseName().Name;

  // Created object will have one sub object.
  std::vector<OpcUa::Model::Object> objects = object.GetObjects();
  ASSERT_EQ(objects.size(), 1);
  const OpcUa::Model::Object& subObject = objects[0];
  // Sub object in the source object type dedn't have any sub objects.
  // But it has a type definition which has one variable.
  // And new instantiated object have to restore full hierarchy.
  std::vector<OpcUa::Model::Variable> variables;
  ASSERT_NO_THROW(variables = subObject.GetVariables());
  ASSERT_EQ(variables.size(), 1);
}

TEST_F(Model, ServerAccessObjectTypes)
{
  OpcUa::Model::Server server(Services);
  OpcUa::Model::ObjectType baseObjectType = server.GetObjectType(OpcUa::ObjectID::BaseObjectType);
  ASSERT_EQ(baseObjectType.GetID(), OpcUa::ObjectID::BaseObjectType);
  ASSERT_EQ(baseObjectType.GetDisplayName().Text, OpcUa::Names::BaseObjectType);
}
