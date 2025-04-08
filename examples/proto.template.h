namespace test
{
    {{#classes}}
    struct {{name}}
    {
        {{#members}}
        {{type}} {{name}}{{#default}} = {{.}}{{/default}};
        {{/members}}
    };
    {{/classes}}
}
