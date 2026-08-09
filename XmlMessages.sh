function get_files
{
    echo file-integration/basket.xml
}

function po_for_file
{
    case "$1" in
       file-integration/basket.xml)
           echo basket_xml_mimetypes.po
       ;;
    esac
}

function tags_for_file
{
    case "$1" in
       file-integration/basket.xml)
           echo comment
       ;;
    esac
}
