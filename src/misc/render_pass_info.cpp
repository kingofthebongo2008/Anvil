//
// Copyright (c) 2017 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "misc/render_pass_info.h"
#include "wrappers/device.h"
#include <algorithm>
#include <cmath>


Anvil::RenderPassInfo::RenderPassInfo(std::weak_ptr<Anvil::BaseDevice> in_device_ptr)
    :m_device_ptr                  (in_device_ptr),
     m_update_preserved_attachments(false)
{
    anvil_assert(in_device_ptr.lock() != nullptr);
}

Anvil::RenderPassInfo::~RenderPassInfo()
{
    /* Stub */
}

/* Please see haeder for specification */
bool Anvil::RenderPassInfo::add_color_attachment(VkFormat                in_format,
                                                    VkSampleCountFlags      in_sample_count,
                                                    VkAttachmentLoadOp      in_load_op,
                                                    VkAttachmentStoreOp     in_store_op,
                                                    VkImageLayout           in_initial_layout,
                                                    VkImageLayout           in_final_layout,
                                                    bool                    in_may_alias,
                                                    RenderPassAttachmentID* out_attachment_id_ptr)
{
    uint32_t new_attachment_index = UINT32_MAX;
    bool     result               = false;

    if (out_attachment_id_ptr == nullptr)
    {
        anvil_assert(out_attachment_id_ptr != nullptr);

        goto end;
    }

    new_attachment_index   = static_cast<uint32_t>(m_attachments.size() );
    *out_attachment_id_ptr = new_attachment_index;

    m_attachments.push_back(RenderPassAttachment(in_format,
                                                 in_sample_count,
                                                 in_load_op,
                                                 in_store_op,
                                                 in_initial_layout,
                                                 in_final_layout,
                                                 in_may_alias,
                                                 new_attachment_index) );

    result = true;

end:
    return result;
}

/** Adds a new dependency to the internal data model.
 *
 *  @param in_destination_subpass_ptr Pointer to the descriptor of the destination subpass.
 *                                    If nullptr, it is assumed an external destination is requested.
 *  @param in_source_subpass_ptr      Pointer to the descriptor of the source subpass.
 *                                    If nullptr, it is assumed an external source is requested.
 *  @param in_source_stage_mask       Source pipeline stage mask.
 *  @param in_destination_stage_mask  Destination pipeline stage mask.
 *  @param in_source_access_mask      Source access mask.
 *  @param in_destination_access_mask Destination access mask.
 *  @param in_by_region               true if a "by-region" dependency is requested; false otherwise.
 *
 *  @return true if the dependency was added successfully; false otherwise.
 *
 **/
bool Anvil::RenderPassInfo::add_dependency(SubPass*             in_destination_subpass_ptr,
                                              SubPass*             in_source_subpass_ptr,
                                              VkPipelineStageFlags in_source_stage_mask,
                                              VkPipelineStageFlags in_destination_stage_mask,
                                              VkAccessFlags        in_source_access_mask,
                                              VkAccessFlags        in_destination_access_mask,
                                              bool                 in_by_region)
{
    auto new_dep = SubPassDependency(in_destination_stage_mask,
                                     in_destination_subpass_ptr,
                                     in_source_stage_mask,
                                     in_source_subpass_ptr,
                                     in_source_access_mask,
                                     in_destination_access_mask,
                                     in_by_region);

    if (std::find(m_subpass_dependencies.begin(),
                  m_subpass_dependencies.end(),
                  new_dep) == m_subpass_dependencies.end() )
    {
        m_subpass_dependencies.push_back(new_dep);
    }

    return true;
}

/* Please see header for specification */
bool Anvil::RenderPassInfo::add_depth_stencil_attachment(VkFormat                in_format,
                                                            VkSampleCountFlags      in_sample_count,
                                                            VkAttachmentLoadOp      in_depth_load_op,
                                                            VkAttachmentStoreOp     in_depth_store_op,
                                                            VkAttachmentLoadOp      in_stencil_load_op,
                                                            VkAttachmentStoreOp     in_stencil_store_op,
                                                            VkImageLayout           in_initial_layout,
                                                            VkImageLayout           in_final_layout,
                                                            bool                    in_may_alias,
                                                            RenderPassAttachmentID* out_attachment_id_ptr)
{
    uint32_t new_attachment_index = UINT32_MAX;
    bool     result               = false;

    if (out_attachment_id_ptr == nullptr)
    {
        anvil_assert(out_attachment_id_ptr != nullptr);

        goto end;
    }

    new_attachment_index   = static_cast<uint32_t>(m_attachments.size() );
    *out_attachment_id_ptr = new_attachment_index;

    m_attachments.push_back(RenderPassAttachment(in_format,
                                                 in_sample_count,
                                                 in_depth_load_op,
                                                 in_depth_store_op,
                                                 in_stencil_load_op,
                                                 in_stencil_store_op,
                                                 in_initial_layout,
                                                 in_final_layout,
                                                 in_may_alias,
                                                 new_attachment_index) );

    result = true;
end:
    return result;
}

/* Please see header for specification */
bool Anvil::RenderPassInfo::add_external_to_subpass_dependency(SubPassID            in_destination_subpass_id,
                                                                  VkPipelineStageFlags in_source_stage_mask,
                                                                  VkPipelineStageFlags in_destination_stage_mask,
                                                                  VkAccessFlags        in_source_access_mask,
                                                                  VkAccessFlags        in_destination_access_mask,
                                                                  bool                 in_by_region)
{
    SubPass* destination_subpass_ptr = nullptr;
    bool     result                  = false;

    if (m_subpasses.size() <= in_destination_subpass_id)
    {
        anvil_assert(!(m_subpasses.size() <= in_destination_subpass_id) );

        goto end;
    }

    destination_subpass_ptr = m_subpasses[in_destination_subpass_id].get();

    result = add_dependency(destination_subpass_ptr,
                            nullptr, /* source_subpass_ptr */
                            in_source_stage_mask,
                            in_destination_stage_mask,
                            in_source_access_mask,
                            in_destination_access_mask,
                            in_by_region);
end:
    return result;
}

/* Please see header for specification */
bool Anvil::RenderPassInfo::add_self_subpass_dependency(SubPassID            in_destination_subpass_id,
                                                           VkPipelineStageFlags in_source_stage_mask,
                                                           VkPipelineStageFlags in_destination_stage_mask,
                                                           VkAccessFlags        in_source_access_mask,
                                                           VkAccessFlags        in_destination_access_mask,
                                                           bool                 in_by_region)
{
    SubPass* destination_subpass_ptr = nullptr;
    bool     result                  = false;

    if (m_subpasses.size() <= in_destination_subpass_id)
    {
        anvil_assert(!(m_subpasses.size() <= in_destination_subpass_id) );

        goto end;
    }

    destination_subpass_ptr = m_subpasses[in_destination_subpass_id].get();
    result                  = add_dependency(destination_subpass_ptr,
                                             destination_subpass_ptr,
                                             in_source_stage_mask,
                                             in_destination_stage_mask,
                                             in_source_access_mask,
                                             in_destination_access_mask,
                                             in_by_region);
end:
    return result;
}

/* Please see header for specification */
bool Anvil::RenderPassInfo::add_subpass(SubPassID* out_subpass_id_ptr)

{
    uint32_t new_subpass_index = UINT32_MAX;
    bool     result            = false;

    if (out_subpass_id_ptr == nullptr)
    {
        anvil_assert(out_subpass_id_ptr != nullptr);

        goto end;
    }

    new_subpass_index = static_cast<uint32_t>(m_subpasses.size() );

    /* Spawn a new descriptor */
    {
        std::unique_ptr<SubPass> new_subpass_ptr(
            new SubPass(new_subpass_index)
        );

        m_subpasses.push_back(
            std::move(new_subpass_ptr)
        );

        *out_subpass_id_ptr = new_subpass_index;
    }

    result = true;
end:
    return result;
}

/** Adds a new attachment to the specified subpass.
 *
 *  @param in_subpass_id            ID of the subpass to update. The subpass must have been earlier
 *                                  created with an add_subpass() call.
 *  @param in_is_color_attachment   true if the added attachment is a color attachment;false if it's
 *                                  an input attachment.
 *  @param in_layout                Layout to use for the attachment when executing the subpass.
 *                                  Driver takes care of transforming the attachment to the requested layout
 *                                  before subpass commands starts executing.
 *  @param in_attachment_id         ID of a render-pass attachment ID this sub-pass attachment should
 *                                  refer to.
 *  @param in_attachment_location   Location, under which the specified attachment should be accessible.
 *  @param in_should_resolve        true if the specified attachment is multisample and should be
 *                                  resolved at the end of the sub-pass.
 *  @parma in_resolve_attachment_id If @param should_resolve is true, this argument should specify the
 *                                  ID of a render-pass attachment, to which the resolved data should
 *                                  written to.
 *
 *  @return true if the function executed successfully, false otherwise.
 *
 **/
bool Anvil::RenderPassInfo::add_subpass_attachment(SubPassID              in_subpass_id,
                                                      bool                   in_is_color_attachment,
                                                      VkImageLayout          in_layout,
                                                      RenderPassAttachmentID in_attachment_id,
                                                      uint32_t               in_attachment_location,
                                                      bool                   in_should_resolve,
                                                      RenderPassAttachmentID in_resolve_attachment_id)
{
    RenderPassAttachment*           renderpass_attachment_ptr = nullptr;
    RenderPassAttachment*           resolve_attachment_ptr    = nullptr;
    bool                            result                    = false;
    LocationToSubPassAttachmentMap* subpass_attachments_ptr   = nullptr;
    SubPass*                        subpass_ptr               = nullptr;

    /* Retrieve the subpass descriptor */
    if (in_subpass_id >= m_subpasses.size() )
    {
        anvil_assert(!(in_subpass_id >= m_subpasses.size() ));

        goto end;
    }
    else
    {
        subpass_ptr = m_subpasses.at(in_subpass_id).get();
    }

    /* Retrieve the renderpass attachment descriptor */
    if (in_attachment_id >= m_attachments.size() )
    {
        anvil_assert(!(in_attachment_id >= m_attachments.size()) );

        goto end;
    }
    else
    {
        renderpass_attachment_ptr = &m_attachments.at(in_attachment_id);
    }

    /* Retrieve the resolve attachment descriptor, if one was requested */
    if (in_should_resolve)
    {
        if (in_resolve_attachment_id >= m_attachments.size() )
        {
            anvil_assert(!(in_resolve_attachment_id >= m_attachments.size()) );

            goto end;
        }
        else
        {
            resolve_attachment_ptr = &m_attachments.at(in_resolve_attachment_id);
        }
    }

    /* Make sure the attachment location is not already assigned an attachment */
    subpass_attachments_ptr = (in_is_color_attachment) ? &subpass_ptr->color_attachments_map
                                                       : &subpass_ptr->input_attachments_map;

    if (subpass_attachments_ptr->find(in_attachment_location) != subpass_attachments_ptr->end() )
    {
        anvil_assert(!(subpass_attachments_ptr->find(in_attachment_location) != subpass_attachments_ptr->end()) );

        goto end;
    }

    /* Add the attachment */
    (*subpass_attachments_ptr)[in_attachment_location] = SubPassAttachment(renderpass_attachment_ptr,
                                                                           in_layout,
                                                                           resolve_attachment_ptr);

    if (in_should_resolve)
    {
        anvil_assert(resolve_attachment_ptr != nullptr);

        subpass_ptr->resolved_attachments_map[in_attachment_location] = SubPassAttachment(resolve_attachment_ptr,
                                                                                          in_layout,
                                                                                          nullptr);
    }

    m_update_preserved_attachments = true;
    result                         = true;

end:
    return result;
}

/* Please see header for specification */
bool Anvil::RenderPassInfo::add_subpass_color_attachment(SubPassID                     in_subpass_id,
                                                            VkImageLayout                 in_input_layout,
                                                            RenderPassAttachmentID        in_attachment_id,
                                                            uint32_t                      in_location,
                                                            const RenderPassAttachmentID* in_attachment_resolve_id_ptr)
{
    return add_subpass_attachment(in_subpass_id,
                                  true, /* is_color_attachment */
                                  in_input_layout,
                                  in_attachment_id,
                                  in_location,
                                  (in_attachment_resolve_id_ptr != nullptr),
                                  (in_attachment_resolve_id_ptr != nullptr) ? *in_attachment_resolve_id_ptr
                                                                            : UINT32_MAX);
}

/* Please see header for specification */
bool Anvil::RenderPassInfo::add_subpass_depth_stencil_attachment(SubPassID              in_subpass_id,
                                                                 RenderPassAttachmentID in_attachment_id,
                                                                 VkImageLayout          in_layout)
{
    RenderPassAttachment* ds_attachment_ptr      = nullptr;
    RenderPassAttachment* resolve_attachment_ptr = nullptr;
    bool                  result                 = false;
    SubPass*              subpass_ptr            = nullptr;

    /* Retrieve the subpass descriptor */
    if (m_subpasses.size() <= in_subpass_id)
    {
        anvil_assert(!(m_subpasses.size() <= in_subpass_id) );

        goto end;
    }
    else
    {
        subpass_ptr = m_subpasses.at(in_subpass_id).get();
    }

    /* Retrieve the attachment descriptors */
    if (m_attachments.size() <= in_attachment_id)
    {
        anvil_assert(!(m_attachments.size() <= in_attachment_id) );

        goto end;
    }
    else
    {
        ds_attachment_ptr = &m_attachments.at(in_attachment_id);
    }

    /* Update the depth/stencil attachment for the subpass */
    if (subpass_ptr->depth_stencil_attachment.attachment_ptr != nullptr)
    {
        anvil_assert(!(subpass_ptr->depth_stencil_attachment.attachment_ptr != nullptr) );

        goto end;
    }

    subpass_ptr->depth_stencil_attachment = SubPassAttachment(ds_attachment_ptr,
                                                              in_layout,
                                                              resolve_attachment_ptr);

    m_update_preserved_attachments = true;
    result                         = true;
end:
    return result;
}

/* Please see header for specification */
bool Anvil::RenderPassInfo::add_subpass_input_attachment(SubPassID              in_subpass_id,
                                                            VkImageLayout          in_layout,
                                                            RenderPassAttachmentID in_attachment_id,
                                                            uint32_t               in_attachment_index)
{
    return add_subpass_attachment(in_subpass_id,
                                  false, /* is_color_attachment */
                                  in_layout,
                                  in_attachment_id,
                                  in_attachment_index,
                                  false,         /* should_resolve        */
                                  UINT32_MAX);   /* resolve_attachment_id */
}

/* Please see header for specification */
bool Anvil::RenderPassInfo::add_subpass_to_external_dependency(SubPassID            in_source_subpass_id,
                                                                  VkPipelineStageFlags in_source_stage_mask,
                                                                  VkPipelineStageFlags in_destination_stage_mask,
                                                                  VkAccessFlags        in_source_access_mask,
                                                                  VkAccessFlags        in_destination_access_mask,
                                                                  bool                 in_by_region)
{
    bool     result             = false;
    SubPass* source_subpass_ptr = nullptr;

    if (m_subpasses.size() <= in_source_subpass_id)
    {
        anvil_assert(!(m_subpasses.size() <= in_source_subpass_id) );

        goto end;
    }

    source_subpass_ptr = m_subpasses[in_source_subpass_id].get();

    result = add_dependency(nullptr, /* destination_subpass_ptr */
                            source_subpass_ptr,
                            in_source_stage_mask,
                            in_destination_stage_mask,
                            in_source_access_mask,
                            in_destination_access_mask,
                            in_by_region);
end:
    return result;
}

/* Please see header for specification */
bool Anvil::RenderPassInfo::add_subpass_to_subpass_dependency(SubPassID            in_source_subpass_id,
                                                                 SubPassID            in_destination_subpass_id,
                                                                 VkPipelineStageFlags in_source_stage_mask,
                                                                 VkPipelineStageFlags in_destination_stage_mask,
                                                                 VkAccessFlags        in_source_access_mask,
                                                                 VkAccessFlags        in_destination_access_mask,
                                                                 bool                 in_by_region)
{
    SubPass* destination_subpass_ptr = nullptr;
    bool     result                  = false;
    SubPass* source_subpass_ptr      = nullptr;

    if (m_subpasses.size() <= in_destination_subpass_id)
    {
        anvil_assert(!(m_subpasses.size() <= in_destination_subpass_id) );

        goto end;
    }

    if (m_subpasses.size() <= in_source_subpass_id)
    {
        anvil_assert(!(m_subpasses.size() <= in_source_subpass_id) );

        goto end;
    }

    destination_subpass_ptr = m_subpasses[in_destination_subpass_id].get();
    source_subpass_ptr      = m_subpasses[in_source_subpass_id].get();

    result = add_dependency(destination_subpass_ptr,
                            source_subpass_ptr,
                            in_source_stage_mask,
                            in_destination_stage_mask,
                            in_source_access_mask,
                            in_destination_access_mask,
                            in_by_region);
end:
    return result;
}

/** Creates a VkAttachmentReference descriptor from the specified RenderPassAttachment instance.
 *
 *  @param in_renderpass_attachment Renderpass attachment descriptor to create the Vulkan descriptor from.
 *
 *  @return As per description.
 **/
VkAttachmentReference Anvil::RenderPassInfo::get_attachment_reference_from_renderpass_attachment(const RenderPassAttachment& in_renderpass_attachment) const
{
    VkAttachmentReference attachment_vk;

    attachment_vk.attachment = in_renderpass_attachment.index;
    attachment_vk.layout     = in_renderpass_attachment.initial_layout;

    return attachment_vk;
}

/** Creates a VkAttachmentReference descriptor from the specified SubPassAttachment instance.
 *
 *  @param in_renderpass_attachment Subpass attachment descriptor to create the Vulkan descriptor from.
 *
 *  @return As per description.
 **/
VkAttachmentReference Anvil::RenderPassInfo::get_attachment_reference_from_subpass_attachment(const SubPassAttachment& in_subpass_attachment) const
{
    VkAttachmentReference attachment_vk;

    attachment_vk.attachment = in_subpass_attachment.attachment_ptr->index;
    attachment_vk.layout     = in_subpass_attachment.layout;

    return attachment_vk;
}

/** Creates a VkAttachmentReference descriptor for a resolve attachment for a color attachment specified by the user.
 *
 *  @param in_subpass_iterator                     Iterator pointing at the subpass which uses the color attachment of interest.
 *  @param in_location_to_subpass_att_map_iterator Iterator pointing at a color attachment which has a resolve attachment attached.
 *
 *  @return As per description.
 **/
VkAttachmentReference Anvil::RenderPassInfo::get_attachment_reference_for_resolve_attachment(const SubPassesConstIterator&                      in_subpass_iterator,
                                                                                                const LocationToSubPassAttachmentMapConstIterator& in_location_to_subpass_att_map_iterator) const
{
    VkAttachmentReference result;

    anvil_assert((*in_subpass_iterator)->resolved_attachments_map.find(in_location_to_subpass_att_map_iterator->first) != (*in_subpass_iterator)->resolved_attachments_map.end() );

    result.attachment = in_location_to_subpass_att_map_iterator->second.resolve_attachment_ptr->index;
    result.layout     = (*in_subpass_iterator)->resolved_attachments_map.at(in_location_to_subpass_att_map_iterator->first).layout;

    return result;
}

/* Please see header for specification */
bool Anvil::RenderPassInfo::get_attachment_type(RenderPassAttachmentID in_attachment_id,
                                                AttachmentType*        out_attachment_type_ptr) const
{
    bool result = false;

    if (m_attachments.size() <= in_attachment_id)
    {
        anvil_assert(m_attachments.size() > in_attachment_id);

        goto end;
    }

    *out_attachment_type_ptr = m_attachments[in_attachment_id].type;

    /* All done */
    result = true;
end:
    return result;
}

/* Please see header for specification */
bool Anvil::RenderPassInfo::get_color_attachment_properties(RenderPassAttachmentID in_attachment_id,
                                                            VkSampleCountFlagBits* out_opt_sample_count_ptr,
                                                            VkAttachmentLoadOp*    out_opt_load_op_ptr,
                                                            VkAttachmentStoreOp*   out_opt_store_op_ptr,
                                                            VkImageLayout*         out_opt_initial_layout_ptr,
                                                            VkImageLayout*         out_opt_final_layout_ptr,
                                                            bool*                  out_opt_may_alias_ptr) const
{
    bool result = false;

    if (m_attachments.size() <= in_attachment_id)
    {
        goto end;
    }

    if (out_opt_sample_count_ptr != nullptr)
    {
        *out_opt_sample_count_ptr = static_cast<VkSampleCountFlagBits>(m_attachments[in_attachment_id].sample_count);
    }

    if (out_opt_load_op_ptr != nullptr)
    {
        *out_opt_load_op_ptr = m_attachments[in_attachment_id].color_depth_load_op;
    }

    if (out_opt_store_op_ptr != nullptr)
    {
        *out_opt_store_op_ptr = m_attachments[in_attachment_id].color_depth_store_op;
    }

    if (out_opt_initial_layout_ptr != nullptr)
    {
        *out_opt_initial_layout_ptr = m_attachments[in_attachment_id].initial_layout;
    }

    if (out_opt_final_layout_ptr != nullptr)
    {
        *out_opt_final_layout_ptr = m_attachments[in_attachment_id].final_layout;
    }

    if (out_opt_may_alias_ptr != nullptr)
    {
        *out_opt_may_alias_ptr = m_attachments[in_attachment_id].may_alias;
    }

    result = true;
end:
    return result;
}

/** Please see header for specification */
bool Anvil::RenderPassInfo::get_dependency_properties(uint32_t              in_n_dependency,
                                                      SubPassID*            out_destination_subpass_id_ptr,
                                                      SubPassID*            out_source_subpass_id_ptr,
                                                      VkPipelineStageFlags* out_destination_stage_mask_ptr,
                                                      VkPipelineStageFlags* out_source_stage_mask_ptr,
                                                      VkAccessFlags*        out_destination_access_mask_ptr,
                                                      VkAccessFlags*        out_source_access_mask_ptr,
                                                      bool*                 out_by_region_ptr) const
{
    const Anvil::RenderPassInfo::SubPassDependency* dep_ptr = nullptr;
    bool                                            result  = false;

    if (m_subpass_dependencies.size() <= in_n_dependency)
    {
        anvil_assert_fail();

        goto end;
    }

    dep_ptr = &m_subpass_dependencies[in_n_dependency];

    *out_destination_subpass_id_ptr  = (dep_ptr->destination_subpass_ptr != nullptr) ? dep_ptr->destination_subpass_ptr->index
                                                                                     : UINT32_MAX;
    *out_source_subpass_id_ptr       = (dep_ptr->source_subpass_ptr      != nullptr) ? dep_ptr->source_subpass_ptr->index
                                                                                     : UINT32_MAX;
    *out_destination_stage_mask_ptr  = dep_ptr->destination_stage_mask;
    *out_source_stage_mask_ptr       = dep_ptr->source_stage_mask;
    *out_destination_access_mask_ptr = dep_ptr->destination_access_mask;
    *out_source_access_mask_ptr      = dep_ptr->source_access_mask;
    *out_by_region_ptr               = dep_ptr->by_region;

    /* All done */
    result = true;
end:
    return result;
}

/** Please see header for specification */
bool Anvil::RenderPassInfo::get_depth_stencil_attachment_properties(RenderPassAttachmentID in_attachment_id,
                                                                    VkAttachmentLoadOp*    out_opt_depth_load_op_ptr,
                                                                    VkAttachmentStoreOp*   out_opt_depth_store_op_ptr,
                                                                    VkAttachmentLoadOp*    out_opt_stencil_load_op_ptr,
                                                                    VkAttachmentStoreOp*   out_opt_stencil_store_op_ptr,
                                                                    VkImageLayout*         out_opt_initial_layout_ptr,
                                                                    VkImageLayout*         out_opt_final_layout_ptr,
                                                                    bool*                  out_opt_may_alias_ptr) const
{
    bool result = false;

    if (m_attachments.size() <= in_attachment_id)
    {
        goto end;
    }

    if (out_opt_depth_load_op_ptr != nullptr)
    {
        *out_opt_depth_load_op_ptr = m_attachments[in_attachment_id].color_depth_load_op;
    }

    if (out_opt_depth_store_op_ptr != nullptr)
    {
        *out_opt_depth_store_op_ptr = m_attachments[in_attachment_id].color_depth_store_op;
    }

    if (out_opt_stencil_load_op_ptr != nullptr)
    {
        *out_opt_stencil_load_op_ptr = m_attachments[in_attachment_id].stencil_load_op;
    }

    if (out_opt_stencil_store_op_ptr != nullptr)
    {
        *out_opt_stencil_store_op_ptr = m_attachments[in_attachment_id].stencil_store_op;
    }

    if (out_opt_initial_layout_ptr != nullptr)
    {
        *out_opt_initial_layout_ptr = m_attachments[in_attachment_id].initial_layout;
    }

    if (out_opt_final_layout_ptr != nullptr)
    {
        *out_opt_final_layout_ptr = m_attachments[in_attachment_id].final_layout;
    }

    if (out_opt_may_alias_ptr != nullptr)
    {
        *out_opt_may_alias_ptr = m_attachments[in_attachment_id].may_alias;
    }

    result = true;
end:
    return result;
}

/* Please see header for specification */
bool Anvil::RenderPassInfo::get_subpass_n_attachments(SubPassID      in_subpass_id,
                                                      AttachmentType in_attachment_type,
                                                      uint32_t*      out_n_attachments_ptr)
{
    bool result = false;

    if (m_subpasses.size() <= in_subpass_id)
    {
        anvil_assert(!(m_subpasses.size() <= in_subpass_id) );

        goto end;
    }
    else
    {
        result = true;

        if (in_attachment_type             == ATTACHMENT_TYPE_PRESERVE &&
            m_update_preserved_attachments)
        {
            update_preserved_attachments();

            anvil_assert(!m_update_preserved_attachments);
        }

        switch (in_attachment_type)
        {
            case ATTACHMENT_TYPE_COLOR:    *out_n_attachments_ptr = static_cast<uint32_t>(m_subpasses[in_subpass_id]->color_attachments_map.size() );    break;
            case ATTACHMENT_TYPE_INPUT:    *out_n_attachments_ptr = static_cast<uint32_t>(m_subpasses[in_subpass_id]->input_attachments_map.size() );    break;
            case ATTACHMENT_TYPE_PRESERVE: *out_n_attachments_ptr = static_cast<uint32_t>(m_subpasses[in_subpass_id]->preserved_attachments.size() );    break;
            case ATTACHMENT_TYPE_RESOLVE:  *out_n_attachments_ptr = static_cast<uint32_t>(m_subpasses[in_subpass_id]->resolved_attachments_map.size() ); break;

            case ATTACHMENT_TYPE_DEPTH_STENCIL:
            {
                *out_n_attachments_ptr = (m_subpasses[in_subpass_id]->depth_stencil_attachment.attachment_ptr != nullptr) ? 1u : 0u;

                break;
            }

            default:
            {
                anvil_assert_fail();

                result = false;
            }
        }
    }

end:
    return result;
}

/* Please see header for specification */
bool Anvil::RenderPassInfo::get_subpass_attachment_properties(SubPassID               in_subpass_id,
                                                              AttachmentType          in_attachment_type,
                                                              uint32_t                in_n_subpass_attachment,
                                                              RenderPassAttachmentID* out_renderpass_attachment_id_ptr,
                                                              VkImageLayout*          out_layout_ptr)
{
    SubPassAttachment               attachment;
    bool                            result                  = false;
    LocationToSubPassAttachmentMap* subpass_attachments_ptr = nullptr;
    SubPass*                        subpass_ptr             = nullptr;

    /* Sanity checks */
    if (in_attachment_type == ATTACHMENT_TYPE_PRESERVE &&
        out_layout_ptr     != nullptr)
    {
        anvil_assert(!(in_attachment_type == ATTACHMENT_TYPE_PRESERVE &&
                       out_layout_ptr     != nullptr) );

        goto end;
    }

    if (m_subpasses.size() <= in_subpass_id)
    {
        anvil_assert(!(m_subpasses.size() <= in_subpass_id) );

        goto end;
    }

    subpass_ptr = m_subpasses[in_subpass_id].get();

    /* Even more sanity checks.. */
    switch (in_attachment_type)
    {
        case ATTACHMENT_TYPE_COLOR:   /* Fall-through */
        case ATTACHMENT_TYPE_INPUT:   /* Fall-through */
        case ATTACHMENT_TYPE_RESOLVE:
        {
            subpass_attachments_ptr = (in_attachment_type == ATTACHMENT_TYPE_COLOR) ? &subpass_ptr->color_attachments_map
                                    : (in_attachment_type == ATTACHMENT_TYPE_INPUT) ? &subpass_ptr->input_attachments_map
                                                                                    : &subpass_ptr->resolved_attachments_map;

            auto iterator = subpass_attachments_ptr->find(in_n_subpass_attachment);

            if (iterator == subpass_attachments_ptr->end() )
            {
                anvil_assert_fail();

                goto end;
            }

            *out_layout_ptr                   = iterator->second.layout;
            *out_renderpass_attachment_id_ptr = iterator->second.attachment_ptr->index;

            break;
        }

        case ATTACHMENT_TYPE_DEPTH_STENCIL:
        {
            if (in_n_subpass_attachment > 0)
            {
                anvil_assert(in_n_subpass_attachment == 0);

                goto end;
            }

            *out_layout_ptr                   = subpass_ptr->depth_stencil_attachment.layout;
            *out_renderpass_attachment_id_ptr = subpass_ptr->depth_stencil_attachment.attachment_ptr->index;

            break;
        }

        case ATTACHMENT_TYPE_PRESERVE:
        {
            if (subpass_ptr->preserved_attachments.size() <= in_n_subpass_attachment)
            {
                anvil_assert(subpass_ptr->preserved_attachments.size() > in_n_subpass_attachment);

                goto end;
            }

            const auto& attachment_props = subpass_ptr->preserved_attachments[in_n_subpass_attachment];

            *out_renderpass_attachment_id_ptr = attachment_props.attachment_ptr->index;
            break;
        }

        default:
        {
            anvil_assert_fail();

            goto end;
        }
    }

    /* All done */
    result = true;

end:
    return result;
}

/** Initializes the vector of preserved attachments for all defined attachments. The algorithm
 *  used is as follows:
 *
 *  For each subpass:
 *
 *  1. Check what the highest subpass index that the attachment is preserved/used in is.
 *  2. For all previous subpasses, determine which subpasses do not use it. For each subpass, preserve
 *     the attachment.
 *
 *  This approach may need to be changed or extended in the future.
 *
 *  This function should be considered expensive.
 **/
void Anvil::RenderPassInfo::update_preserved_attachments()
{
    anvil_assert(m_update_preserved_attachments);

    /* Cache color, depth+stencil, resolve attachments, as used by all defined subpasses.
     * Make sure not to insert duplicate items. */
    std::vector<SubPassAttachment*> unique_attachments;

    for (auto subpass_iterator  = m_subpasses.begin();
              subpass_iterator != m_subpasses.end();
            ++subpass_iterator)
    {
        SubPass* current_subpass_ptr = subpass_iterator->get();

        for (uint32_t n_attachment_type = 0;
                      n_attachment_type < 3; /* color, depth+stencil, resolve */
                    ++n_attachment_type)
        {
            const uint32_t n_attachments = (n_attachment_type == 0) ? static_cast<uint32_t>(current_subpass_ptr->color_attachments_map.size() )
                                         : (n_attachment_type == 1) ? ((current_subpass_ptr->depth_stencil_attachment.attachment_ptr != nullptr) ? 1 : 0)
                                                                    : static_cast<uint32_t>(current_subpass_ptr->resolved_attachments_map.size() );

            for (uint32_t n_attachment = 0;
                          n_attachment < n_attachments;
                        ++n_attachment)
            {
                SubPassAttachment* current_attachment_ptr = (n_attachment_type == 0) ?  current_subpass_ptr->get_color_attachment_at_index(n_attachment)
                                                          : (n_attachment_type == 1) ? &current_subpass_ptr->depth_stencil_attachment
                                                                                     :  current_subpass_ptr->get_resolved_attachment_at_index(n_attachment);

                if (std::find(unique_attachments.begin(),
                              unique_attachments.end(),
                              current_attachment_ptr) == unique_attachments.end() )
                {
                    unique_attachments.push_back(current_attachment_ptr);
                }
            }
        }

        /* Clean subpass's preserved attachments vector along the way.. */
        current_subpass_ptr->preserved_attachments.clear();
    }

    /* Determine what the index of the subpass which uses each unique attachment for the first, and for the last time, is. */
    for (std::vector<SubPassAttachment*>::iterator unique_attachment_iterator  = unique_attachments.begin();
                                                   unique_attachment_iterator != unique_attachments.end();
                                                 ++unique_attachment_iterator)
    {
        uint32_t                 current_subpass_index         = 0;
        const SubPassAttachment* current_unique_attachment_ptr = *unique_attachment_iterator;
        uint32_t                 lowest_subpass_index          = static_cast<uint32_t>(m_subpasses.size() - 1);
        uint32_t                 highest_subpass_index         = 0;

        for (auto subpass_iterator  = m_subpasses.begin();
                  subpass_iterator != m_subpasses.end();
                ++subpass_iterator, ++current_subpass_index)
        {
            SubPass* current_subpass_ptr = subpass_iterator->get();
            bool     subpass_processed   = false;

            for (uint32_t n_attachment_type = 0;
                          n_attachment_type < 3 /* color, depth+stencil, resolve */ && !subpass_processed;
                        ++n_attachment_type)
            {
                const uint32_t n_attachments = (n_attachment_type == 0) ? static_cast<uint32_t>(current_subpass_ptr->color_attachments_map.size() )
                                             : (n_attachment_type == 1) ? ((current_subpass_ptr->depth_stencil_attachment.attachment_ptr != nullptr) ? 1 : 0)
                                                                        : static_cast<uint32_t>(current_subpass_ptr->resolved_attachments_map.size() );

                for (uint32_t n_attachment = 0;
                              n_attachment < n_attachments && !subpass_processed;
                            ++n_attachment)
                {
                    SubPassAttachment* current_attachment_ptr = (n_attachment_type == 0) ?  current_subpass_ptr->get_color_attachment_at_index(n_attachment)
                                                              : (n_attachment_type == 1) ? &current_subpass_ptr->depth_stencil_attachment
                                                                                         :  current_subpass_ptr->get_resolved_attachment_at_index(n_attachment);

                    if (current_attachment_ptr == current_unique_attachment_ptr)
                    {
                        if (lowest_subpass_index > current_subpass_index)
                        {
                            lowest_subpass_index = current_subpass_index;
                        }

                        highest_subpass_index = current_subpass_index;
                        subpass_processed     = true;
                    }
                }
            }
        }

        (*unique_attachment_iterator)->highest_subpass_index  = highest_subpass_index;
        (*unique_attachment_iterator)->lowest_subpass_index   = lowest_subpass_index;
    }

    /* For each unique attachment, add it to the list of preserved attachments for all subpasses that precede the
     * one at the highest subpass index and follow the one at the lowest subpass index, as long as the
     * attachment is not used by a subpass. */
    for (std::vector<SubPassAttachment*>::iterator unique_attachment_iterator  = unique_attachments.begin();
                                                   unique_attachment_iterator != unique_attachments.end();
                                                 ++unique_attachment_iterator)
    {
        const SubPassAttachment* current_unique_attachment_ptr = *unique_attachment_iterator;

        if ((*unique_attachment_iterator)->highest_subpass_index == (*unique_attachment_iterator)->lowest_subpass_index)
        {
            /* There is only one producer subpass and no consumer subpasses defined for this renderpass.
             *No need to create any preserve attachments. */
            continue;
        }

        for (auto subpass_iterator  = (m_subpasses.begin() + static_cast<int>(current_unique_attachment_ptr->lowest_subpass_index) );
                  subpass_iterator != (m_subpasses.begin() + static_cast<int>(current_unique_attachment_ptr->highest_subpass_index) ) + 1;
                ++subpass_iterator)
        {
            SubPass* current_subpass_ptr   = subpass_iterator->get();
            bool     is_subpass_attachment = false;

            for (uint32_t n_attachment_type = 0;
                          n_attachment_type < 3 /* color, depth+stencil, resolve */ && !is_subpass_attachment;
                        ++n_attachment_type)
            {
                const uint32_t n_attachments = (n_attachment_type == 0) ? static_cast<uint32_t>(current_subpass_ptr->color_attachments_map.size() )
                                             : (n_attachment_type == 1) ? ((current_subpass_ptr->depth_stencil_attachment.attachment_ptr != nullptr) ? 1 : 0)
                                                                        : static_cast<uint32_t>(current_subpass_ptr->resolved_attachments_map.size() );

                for (uint32_t n_attachment = 0;
                              n_attachment < n_attachments && !is_subpass_attachment;
                            ++n_attachment)
                {
                    SubPassAttachment* current_attachment_ptr = (n_attachment_type == 0) ?  current_subpass_ptr->get_color_attachment_at_index(n_attachment)
                                                              : (n_attachment_type == 1) ? &current_subpass_ptr->depth_stencil_attachment
                                                                                         :  current_subpass_ptr->get_resolved_attachment_at_index(n_attachment);

                    if (current_attachment_ptr == current_unique_attachment_ptr)
                    {
                        is_subpass_attachment = true;
                    }
                }
            }

            if (!is_subpass_attachment)
            {
                #ifdef _DEBUG
                {
                    bool           is_already_preserved    = false;
                    const uint32_t n_preserved_attachments = static_cast<uint32_t>(current_subpass_ptr->preserved_attachments.size() );

                    for (uint32_t n_preserved_attachment = 0;
                                  n_preserved_attachment < n_preserved_attachments;
                                ++n_preserved_attachment)
                    {
                        if (&current_subpass_ptr->preserved_attachments[n_preserved_attachment] == current_unique_attachment_ptr)
                        {
                            is_already_preserved = true;

                            break;
                        }
                    }

                    anvil_assert(!is_already_preserved);
                }
                #endif

                current_subpass_ptr->preserved_attachments.push_back(*current_unique_attachment_ptr);
            }
        }
    }

    m_update_preserved_attachments = false;
}

/** Please see header for specification */
Anvil::RenderPassInfo::SubPass::~SubPass()
{
    /* Stub */
}